/*
 * Copyright (C) 2018 Konsulko Group
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <glib.h>

#include "rtl_fm.h"
#include "radio_output.h"

#ifdef DEBUG
#define LOG_FILE	"/tmp/helper.log"
FILE *_log;
#define LOG(...) \
	do { \
		if(!_log) _log = fopen(LOG_FILE, "w");	\
		fprintf(_log, __VA_ARGS__);		\
		fflush(_log);				\
	} while(0)
#else
#define LOG(...) do { } while(0)
#endif

// Structure to describe FM band plans, all values in Hz.
typedef struct {
	const char *name;
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
static char line[64];

static void rtl_output_callback(int16_t *result, int result_len, void *ctx)
{
	radio_output_write((char*) result, result_len * 2);
}

static void rtl_freq_callback(uint32_t freq, void *ctx)
{
	// Note, need to flush output to ensure parent sees it
	printf("F=%u\n", freq);
	fflush(stdout);
	LOG("F=%u\n", freq);
}

static void rtl_scan_callback(uint32_t freq, void *ctx)
{
	// Note, need to flush output to ensure parent sees it
	printf("S=%u\n", freq);
	fflush(stdout);
	LOG("S=%u\n", freq);
}

static void read_config(void)
{
	GKeyFile* conf_file;
	int conf_file_present = 0;
	char *value_str;

	// Load settings from configuration file if it exists
	conf_file = g_key_file_new();
	if(conf_file &&
	   g_key_file_load_from_dirs(conf_file,
				     "AGL.conf",
				     (const gchar**) g_get_system_config_dirs(),
				     NULL,
				     G_KEY_FILE_KEEP_COMMENTS,
				     NULL) == TRUE) {
		conf_file_present = 1;

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
	fprintf(stderr, "Using FM Bandplan: %s\n", known_fm_band_plans[bandplan].name);

	if(conf_file_present) {
		GError *error = NULL;
		int n;

		// Allow over-riding scanning parameters just in case a demo
		// setup needs to do so to work reliably.
		n = g_key_file_get_integer(conf_file,
					   "radio",
					   "scan_squelch_level",
					   &error);
		if(!error) {
			fprintf(stderr, "Scanning squelch level set to %d\n", n);
			rtl_fm_scan_set_squelch_level(n);
		}

		error = NULL;
		n = g_key_file_get_integer(conf_file,
					   "radio",
					   "scan_squelch_limit",
					   &error);
		if(!error) {
			fprintf(stderr, "Scanning squelch limit set to %d\n", n);
			rtl_fm_scan_set_squelch_limit(n);
		}

		g_key_file_free(conf_file);
	}
}

int main(int argc, char *argv[])
{
	int rc;
	bool detect = false;
	bool done = false;
	bool started = false;
	uint32_t frequency;

	LOG("started\n");

	read_config();
	frequency = known_fm_band_plans[bandplan].min;

	if(argc == 2 && strcmp(argv[1], "--detect") == 0) {
		detect = true;
	}

	rc = rtl_fm_init(frequency, 200000, 48000, rtl_output_callback, NULL);
	if(rc < 0) {
		fprintf(stderr, "No RTL USB adapter?\n");
		exit(1);
	}
	if(detect) {
		rtl_fm_cleanup();
		exit(0);
	}

	rtl_fm_set_freq_callback(rtl_freq_callback, NULL);

	while(!done) {
		LOG("Reading command\n");
		if (fgets(line, sizeof(line), stdin) == NULL)
			break;
		if(line[0] == '\0' || line[0] == '\n')
			continue;
		if(strcmp(line, "START\n") == 0) {
			LOG("START received\n");
			if(!started) {
				LOG("Starting\n");
				radio_output_start();
				rtl_fm_start();
				started = true;
			}
		} else if(strcmp(line, "STOP\n") == 0) {
			LOG("STOP received\n");
			if(started) {
				LOG("Stopping\n");
				radio_output_stop();
				rtl_fm_stop();
				started = false;
			}
		} else if(strncmp(line, "F=", 2) == 0) {
			uint32_t n;
			if(sscanf(line, "F=%u\n", &n) == 1) {
				LOG("F=%d received\n", n);
				rtl_fm_scan_stop();
				rtl_fm_set_freq(n);
			}
		} else if(strcmp(line, "S=UP\n") == 0) {
			LOG("S=UP received\n");
			if(!started)
				continue;
			LOG("Calling rtl_fm_scan_start\n");
			rtl_fm_scan_start(0,
					  rtl_scan_callback,
					  NULL,
					  known_fm_band_plans[bandplan].step,
					  known_fm_band_plans[bandplan].min,
					  known_fm_band_plans[bandplan].max);
		} else if(strcmp(line, "S=DOWN\n") == 0) {
			LOG("S=DOWN received\n");
			if(!started)
				continue;
			LOG("Calling rtl_fm_scan_start\n");
			rtl_fm_scan_start(1,
					  rtl_scan_callback,
					  NULL,
					  known_fm_band_plans[bandplan].step,
					  known_fm_band_plans[bandplan].min,
					  known_fm_band_plans[bandplan].max);
		} else if(strcmp(line, "S=STOP\n") == 0) {
			LOG("S=STOP received\n");
			if(started) {
				LOG("Calling rtl_fm_scan_stop\n");
				rtl_fm_scan_stop();
			}
		} else if(line[0] == 'q' || line[0] == 'Q')
			break;
	}
	if(started) {
		radio_output_stop();
		rtl_fm_stop();
	}
	rtl_fm_cleanup();
	LOG("done");
	return 0;
}
