/*
 * Copyright (C) 2017,2018 Konsulko Group
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <errno.h>
#include <glib.h>
#include <afb/afb-binding.h>

#include "radio_impl.h"

#define HELPER_NAME	"rtl_fm_helper"
#define HELPER_MAX	PATH_MAX + 64

#define HELPER_CMD_MAXLEN	64
#define HELPER_RSP_MAXLEN	128

// Structure to describe FM band plans, all values in Hz.
typedef struct {
	char *name;
	uint32_t min;
	uint32_t max;
	uint32_t step;
} fm_band_plan_t;

static fm_band_plan_t known_fm_band_plans[5] = {
	{ .name = "US", .min = 87900000, .max = 107900000, .step = 200000 },
	{ .name = "JP", .min = 76000000, .max = 95000000, .step = 100000 },
	{ .name = "EU", .min = 87500000, .max = 108000000, .step = 50000 },
	{ .name = "ITU-1", .min = 87500000, .max = 108000000, .step = 50000 },
	{ .name = "ITU-2", .min = 87900000, .max = 107900000, .step = 50000 }
};

static unsigned int bandplan;
static pid_t helper_pid;
static int helper_in;
static int helper_out;
static pthread_mutex_t helper_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool present;
static bool active;
static bool scanning;
static uint32_t current_frequency;
static radio_freq_callback_t freq_callback;
static void *freq_callback_data;

static uint32_t rtlsdr_get_min_frequency(radio_band_t band);
static void rtlsdr_set_frequency(uint32_t frequency);
static void rtlsdr_scan_stop(void);

/*
 * Bi-directional popen implementation
 * Based on one of the versions given in:
 *
 * https://stackoverflow.com/questions/12778672/killing-process-that-has-been-created-with-popen2
 */
static pid_t popen2(char *command, int *in_fd, int *out_fd)
{
	int     pin[2], pout[2];
	pid_t   pid;

	if(out_fd != NULL) {
		if(pipe(pin) != 0)
			return -1;
	}
	if(in_fd != NULL) {
		if (pipe(pout) != 0) {
			if(out_fd != NULL) {
				close(pin[0]);
				close(pin[1]);
			}
			return -1;
		}
	}

	pid = fork();
	if(pid < 0) {
		if(out_fd != NULL) {
			close(pin[0]);
			close(pin[1]);
		}
		if(in_fd != NULL) {
			close(pout[0]);
			close(pout[1]);
		}
		return pid;
	}
	if(pid == 0) {
		if(out_fd != NULL) {
			close(pin[1]);
			dup2(pin[0], 0);
		}
		if(in_fd != NULL) {
			close(pout[0]);
			dup2(pout[1], 1);
		}
		execlp(command, command, NULL);
		perror("Error:");
		exit(1);
	}
	if(in_fd != NULL) {
		close(pout[1]);
		*in_fd = pout[0];
	}
	if(out_fd != NULL) {
		close(pin[0]);
		*out_fd = pin[1];
	}
	return pid;
}

static int rtlsdr_init(const char *output)
{
	GKeyFile *conf_file;
	char *value_str;
	char *rootdir;
	char *helper_path;

	if(present)
		return 0;

	// Load settings from configuration file if it exists
	conf_file = g_key_file_new();
	if(conf_file &&
	   g_key_file_load_from_dirs(conf_file,
				     "AGL.conf",
				     (const gchar**) g_get_system_config_dirs(),
				     NULL,
				     G_KEY_FILE_KEEP_COMMENTS,
				     NULL) == TRUE) {

		// Set band plan if it is specified
		value_str = g_key_file_get_string(conf_file,
						  "radio",
						  "fmbandplan",
						  NULL);
		if(value_str) {
			unsigned int i;
			for(i = 0;
			    i < sizeof(known_fm_band_plans) / sizeof(fm_band_plan_t);
			    i++) {
				if(!strcasecmp(value_str, known_fm_band_plans[i].name)) {
					bandplan = i;
					break;
				}
			}
		}
	}

	// Start off with minimum bandplan frequency
	current_frequency = rtlsdr_get_min_frequency(BAND_FM);

	rootdir = getenv("AFM_APP_INSTALL_DIR");
	if(!rootdir)
		return -1;

	// Run helper to detect adapter
	helper_path = malloc(HELPER_MAX);
	if(!helper_path)
		return -ENOMEM;
	if(snprintf(helper_path, HELPER_MAX, "%s/bin/%s --detect", rootdir, HELPER_NAME) == HELPER_MAX) {
		AFB_ERROR("Could not create command for \"%s --detect\"", HELPER_NAME);
		return -EINVAL;
	}
	if(system(helper_path) != 0) {
		free(helper_path);
		return -1;
	}

	if(output) {
		// Indicate desired output to helper
		AFB_INFO("Setting RADIO_OUTPUT=%s", output);
		setenv("RADIO_OUTPUT", output, 1);
	}

	// Run helper
	if(snprintf(helper_path, PATH_MAX, "%s/bin/%s", rootdir, HELPER_NAME) == PATH_MAX) {
		AFB_ERROR("Could not create path to %s", HELPER_NAME);
		return -EINVAL;
	}
	helper_pid = popen2(helper_path, &helper_out, &helper_in);
	if(helper_pid < 0) {
		AFB_ERROR("Could not run %s!", HELPER_NAME);
		return -1;
	}
	AFB_DEBUG("%s started", HELPER_NAME);
	free(helper_path);

	present = true;
	rtlsdr_set_frequency(current_frequency);

	return 0;
}

static uint32_t rtlsdr_get_frequency(void)
{
	return current_frequency;
}

static void rtlsdr_set_frequency(uint32_t frequency)
{
	char cmd[HELPER_CMD_MAXLEN];
	char output[HELPER_RSP_MAXLEN];
	bool found = false;
	ssize_t rc;
	uint32_t n;

	if(!present)
		return;

	if(frequency < known_fm_band_plans[bandplan].min ||
	   frequency > known_fm_band_plans[bandplan].max)
		return;

	if(scanning)
		rtlsdr_scan_stop();

	current_frequency = frequency;
	snprintf(cmd, sizeof(cmd), "F=%u\n", frequency);
	pthread_mutex_lock(&helper_mutex);
	rc = write(helper_in, cmd, strlen(cmd));
	if(rc < 0) {
		pthread_mutex_unlock(&helper_mutex);
		return;
	}
	while(!found) {
		rc = read(helper_out, output, sizeof(output)-1);
		if(rc <= 0)
			break;
		output[rc] = '\0';
		if(output[0] == 'F') {
			if(sscanf(output, "F=%u\n", &n) == 1) {
				if(freq_callback)
					freq_callback(n, freq_callback_data);
				found = true;
			}
		}
	}
	pthread_mutex_unlock(&helper_mutex);
}

static void rtlsdr_set_frequency_callback(radio_freq_callback_t callback,
					  void *data)
{
	freq_callback = callback;
	freq_callback_data = data;
}

static radio_band_t rtlsdr_get_band(void)
{
	// We only support FM
	return BAND_FM;
}

static void rtlsdr_set_band(radio_band_t band)
{
	// We only support FM, so do nothing
}

static int rtlsdr_band_supported(radio_band_t band)
{
	if(band == BAND_FM)
		return 1;
	return 0;
}

static uint32_t rtlsdr_get_min_frequency(radio_band_t band)
{
	return known_fm_band_plans[bandplan].min;
}

static uint32_t rtlsdr_get_max_frequency(radio_band_t band)
{
	return known_fm_band_plans[bandplan].max;
}

static uint32_t rtlsdr_get_frequency_step(radio_band_t band)
{
	uint32_t ret = 0;

	switch (band) {
	case BAND_AM:
		ret = 1000; // 1 kHz
		break;
	case BAND_FM:
		ret = known_fm_band_plans[bandplan].step;
		break;
	default:
		break;
	}
	return ret;
}

static void rtlsdr_start(void)
{
	if(!present)
		return;

	if(active)
		return;

	ssize_t rc;
	char cmd[HELPER_CMD_MAXLEN];

	snprintf(cmd, sizeof(cmd), "START\n");
	pthread_mutex_lock(&helper_mutex);
	rc = write(helper_in, cmd, strlen(cmd));
	pthread_mutex_unlock(&helper_mutex);
	if (rc < 0) {
		AFB_ERROR("Failed to ask \"%s\" to start", HELPER_NAME);
		return;
	}
	active = true;
}

static void rtlsdr_stop(void)
{
	ssize_t rc;
	if(!present)
		return;

	if (!active)
		return;

	char cmd[HELPER_CMD_MAXLEN];

	snprintf(cmd, sizeof(cmd), "STOP\n");
	pthread_mutex_lock(&helper_mutex);
	rc = write(helper_in, cmd, strlen(cmd));
	pthread_mutex_unlock(&helper_mutex);
	if (rc < 0) {
		AFB_ERROR("Failed to ask \"%s\" to stop", HELPER_NAME);
		return;
	}
	active = false;
}

static void rtlsdr_scan_start(radio_scan_direction_t direction,
			      radio_scan_callback_t callback,
			      void *data)
{
	ssize_t rc;
	char cmd[HELPER_CMD_MAXLEN];
	char output[HELPER_RSP_MAXLEN];
	bool found = false;
	uint32_t n;

	if(!active || scanning)
		return;

	scanning = true;
	snprintf(cmd,
			 sizeof(cmd),
			 "S=%s\n", direction == SCAN_FORWARD ? "UP" : "DOWN");
	pthread_mutex_lock(&helper_mutex);
	if(!scanning) {
		pthread_mutex_unlock(&helper_mutex);
		return;
	}
	rc = write(helper_in, cmd, strlen(cmd));
	pthread_mutex_unlock(&helper_mutex);
	if(rc < 0)
		return;
	while(!found) {
		pthread_mutex_lock(&helper_mutex);
		if(!scanning) {
			pthread_mutex_unlock(&helper_mutex);
			break;
		}
		rc = read(helper_out, output, sizeof(output)-1);
		pthread_mutex_unlock(&helper_mutex);
		if(rc <= 0)
			break;

		output[rc] = '\0';
		if(output[0] == 'F') {
			if(sscanf(output, "F=%u\n", &n) == 1) {
				current_frequency = n;
				if(freq_callback)
					freq_callback(n, freq_callback_data);
			}
		}
		if(output[0] == 'S') {
			if(sscanf(output, "S=%u\n", &n) == 1) {
				if(callback)
					callback(n, data);
				found = true;
				scanning = false;
			}
		}
	}
}

static void rtlsdr_scan_stop(void)
{
	char cmd[HELPER_CMD_MAXLEN];
	ssize_t rc;

	snprintf(cmd, sizeof(cmd), "S=STOP\n");
	pthread_mutex_lock(&helper_mutex);
	scanning = false;
	rc = write(helper_in, cmd, strlen(cmd));
	pthread_mutex_unlock(&helper_mutex);
	if (rc < 0)
		AFB_ERROR("Failed to ask \"%s\" to stop scan", HELPER_NAME);
}

static radio_stereo_mode_t rtlsdr_get_stereo_mode(void)
{
	// We only support stereo
	return STEREO;
}

static void rtlsdr_set_stereo_mode(radio_stereo_mode_t mode)
{
	// We only support stereo, so do nothing
}

radio_impl_ops_t rtlsdr_impl_ops = {
	.name = "RTL-SDR USB adapter",
	.init = rtlsdr_init,
	.get_frequency = rtlsdr_get_frequency,
	.set_frequency = rtlsdr_set_frequency,
	.set_frequency_callback = rtlsdr_set_frequency_callback,
	.get_band = rtlsdr_get_band,
	.set_band = rtlsdr_set_band,
	.band_supported = rtlsdr_band_supported,
	.get_min_frequency = rtlsdr_get_min_frequency,
	.get_max_frequency = rtlsdr_get_max_frequency,
	.get_frequency_step = rtlsdr_get_frequency_step,
	.start = rtlsdr_start,
	.stop = rtlsdr_stop,
	.scan_start = rtlsdr_scan_start,
	.scan_stop = rtlsdr_scan_stop,
	.get_stereo_mode = rtlsdr_get_stereo_mode,
	.set_stereo_mode = rtlsdr_set_stereo_mode
};
