/*
 * Copyright (C) 2017 Konsulko Group
 * Author: Matt Ranostay <matt.ranostay@konsulko.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <gps.h>
#include <time.h>
#include <pthread.h>
#include <json-c/json.h>
#include <sys/signal.h>
#include <sys/time.h>

#define AFB_BINDING_VERSION 3
#include <afb/afb-binding.h>

static struct gps_data_t data;
static afb_event_t location_event;

static pthread_t thread;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

#define MSECS_TO_USECS(x) (x * 1000)

struct {
	FILE *current_file;
	char buffer[512];
	int replaying;
	int count;
} recording;

// struct dop_t item order
static const char *dop_names[] = {
	"xdop",
	"ydop",
	"pdop",
	"hdop",
	"vdop",
	"tdop",
	"gdop",
	NULL
};

static json_object *populate_json_dop_data(json_object *jresp, struct dop_t *dop)
{
	char **names = (char **) dop_names;
	double *tmp = (double *) dop;
	json_object *value = NULL;

	while (*names) {
		double val = *tmp++;

		if (val != 0) {
			value = json_object_new_double(val);
			json_object_object_add(jresp, *names, value);
		}
		names++;
	}

	return jresp;
}

static json_object *populate_json_data(json_object *jresp)
{
	json_object *value = NULL;

	if (recording.replaying) {
		return json_tokener_parse(recording.buffer);
	}

	if (data.fix.mode != MODE_3D) {
		json_object_put(jresp);
		return NULL;
	}

	if (data.set & ALTITUDE_SET) {
		value = json_object_new_double(data.fix.altitude);
		json_object_object_add(jresp, "altitude", value);
	}

	if (data.set & LATLON_SET) {
		value = json_object_new_double(data.fix.latitude);
		json_object_object_add(jresp, "latitude", value);

		value = json_object_new_double(data.fix.longitude);
		json_object_object_add(jresp, "longitude", value);
	}

	if (data.set & SPEED_SET) {
		value = json_object_new_double(data.fix.speed);
		json_object_object_add(jresp, "speed", value);
	}

	if (data.set & TRACK_SET) {
		value = json_object_new_double(data.fix.track);
		json_object_object_add(jresp, "track", value);
	}

	if (data.set & TIME_SET) {
		char time[30];
		unix_to_iso8601(data.fix.time, (char *) &time, sizeof(time));
		value = json_object_new_string(time);
		json_object_object_add(jresp, "timestamp", value);
	}

	jresp = populate_json_dop_data(jresp, &data.dop);

	return jresp;
}

static void get_data(afb_req_t request)
{
	json_object *jresp = NULL;
	pthread_mutex_lock(&mutex);

	jresp = populate_json_data(json_object_new_object());
	if (jresp != NULL) {
		afb_req_success(request, jresp, "GNSS location data");
	} else {
		afb_req_fail(request, "failed", "No 3D GNSS fix");
	}

	pthread_mutex_unlock(&mutex);
}

static json_object *gps_recording_state(json_object *jresp)
{
	json_object *value = NULL;

	if (recording.current_file == NULL) {
		value = json_object_new_boolean(true);
		json_object_object_add(jresp, "recording", value);
	} else {
		value = json_object_new_boolean(true);
		json_object_object_add(jresp, "recording", value);

		value = json_object_new_string(recording.buffer);
		json_object_object_add(jresp, "filename", value);

		if (recording.count) {
			value = json_object_new_int(recording.count);
			json_object_object_add(jresp, "count", value);
		}
	}

	return jresp;
}

static void record(afb_req_t request)
{
	json_object *jresp = NULL;
	const char *value = afb_req_value(request, "state");
	bool record = false;

	if (recording.replaying) {
		afb_req_fail(request, "failed", "Current in replaying mode");
		return;
	}

	if (!value) {
		pthread_mutex_lock(&mutex);
		jresp = gps_recording_state(json_object_new_object());
		pthread_mutex_unlock(&mutex);

		afb_req_success(request, jresp, "GPS Current Recording State");
		return;
	}

	if (!strcasecmp(value, "on"))
		record = true;
	else if (!strcasecmp(value, "off"))
		record  = false;
	else {
		afb_req_fail(request, "failed", "Invalid state requested");
		return;
	}

	pthread_mutex_lock(&mutex);

	if (recording.current_file != NULL) {
		recording.count = 0;
		fclose(recording.current_file);
		recording.current_file = NULL;

		if (!record) {
			pthread_mutex_unlock(&mutex);
			afb_req_success(request, NULL, NULL);
			return;
		}
	} else if (recording.current_file == NULL && !record) {
		pthread_mutex_unlock(&mutex);
		afb_req_fail(request, "failed", "Recording was already stopped");
		return;
	}

	{
		time_t cur_time;
		struct tm *info;

		time(&cur_time);
		info = localtime(&cur_time);
		strftime((char *) &recording.buffer, sizeof(recording.buffer),
						 "gps_%Y%m%d_%H%M.log", info);

		recording.current_file = fopen(recording.buffer, "w+");

		jresp = gps_recording_state(json_object_new_object());
		afb_req_success(request, jresp, "GPS Recording Result");
	}

	pthread_mutex_unlock(&mutex);
}


static void subscribe(afb_req_t request)
{
	const char *value = afb_req_value(request, "value");

	if (value && !strcasecmp(value, "location")) {
		afb_req_subscribe(request, location_event);
		afb_req_success(request, NULL, NULL);
		return;
	}

	afb_req_fail(request, "failed", "Invalid event");
}

static void unsubscribe(afb_req_t request)
{
	const char *value = afb_req_value(request, "value");

	if (value && !strcasecmp(value, "location")) {
		afb_req_unsubscribe(request, location_event);
		afb_req_success(request, NULL, NULL);
		return;
	}

	afb_req_fail(request, "failed", "Invalid event");
}

static void add_record(json_object *jresp)
{
	fprintf(recording.current_file, "%s\n", json_object_to_json_string(jresp));
	fflush(recording.current_file);

	recording.count++;
}

static void *data_poll(void *ptr)
{
	/*
	 * keep reading till an error condition happens
	 */
	while (gps_waiting(&data, MSECS_TO_USECS(2000)) && !errno)
	{
		json_object *jresp = NULL;

		pthread_mutex_lock(&mutex);
		if (gps_read(&data) == -1) {
			AFB_ERROR("Cannot read from GPS daemon.\n");
			pthread_mutex_unlock(&mutex);
			break;
		}

		if (!(data.set & (TRACKERR_SET | SPEEDERR_SET| CLIMBERR_SET))) {
			jresp = populate_json_data(json_object_new_object());

			if (jresp != NULL) {
				if (recording.current_file != NULL)
					add_record(jresp);

				afb_event_push(location_event, jresp);
			}
		}
		pthread_mutex_unlock(&mutex);
	}

	AFB_INFO("Closing GPS daemon connection.\n");
	gps_stream(&data, WATCH_DISABLE, NULL);
	gps_close(&data);

	_exit(0);

	return NULL;
}

static void replay_thread(union sigval arg)
{
	FILE *fp = recording.current_file;
	char *s = fgets(recording.buffer, sizeof(recording.buffer), fp);

	if (s == NULL) {
		if (feof(fp))
			rewind(fp);
		return;
	}

	afb_event_push(location_event, json_tokener_parse(s));
}

static int gps_init()
{
	const char *host, *port;
	int ret, tries = 5;

	host = getenv("AFBGPS_HOST") ? : "localhost";
	port = getenv("AFBGPS_SERVICE") ? : "2947";

	ret = gps_open(host, port, &data);
	if (ret < 0)
		return ret;

	gps_stream(&data, WATCH_ENABLE | WATCH_JSON, NULL);

	// due to the gpsd.socket race condition need to loop till initial event
	do {
		gps_read(&data);
	} while (!gps_waiting(&data, MSECS_TO_USECS(2500)) && tries--);

	return pthread_create(&thread, NULL, &data_poll, NULL);
}

static int replay_init()
{
	timer_t timer_id;
	struct itimerspec ts;
	struct sigevent se;
	int ret;

	recording.current_file = fopen("recording.log", "r");
	if (recording.current_file == NULL)
		return -EINVAL;

	se.sigev_notify = SIGEV_THREAD;
	se.sigev_value.sival_ptr = &timer_id;
	se.sigev_notify_function = replay_thread;
	se.sigev_notify_attributes = NULL;

	// TODO: Allow detecting of an non-static 1 Hz GPS trace log

	ts.it_value.tv_sec = 1;
	ts.it_value.tv_nsec = 0;

	ts.it_interval.tv_sec = 1;
	ts.it_interval.tv_nsec = 0;

	ret = timer_create(CLOCK_REALTIME, &se, &timer_id);
	if (ret < 0)
		return ret;

	return timer_settime(timer_id, 0, &ts, NULL);
}

/*
 * Test to see if in demo mode first, then enable if not enable gpsd
 */

static int init(afb_api_t api)
{
	int ret;
	location_event = afb_daemon_make_event("location");

	ret = replay_init();

	if (!ret)
		recording.replaying = 1;
	else
		gps_init();

	return 0;
}

static const struct afb_verb_v3 binding_verbs[] = {
	{ .verb = "location",    .callback = get_data,     .info = "Get GNSS data" },
	{ .verb = "record",	 .callback = record,       .info = "Record GPS data" },
	{ .verb = "subscribe",   .callback = subscribe,    .info = "Subscribe to GNSS events" },
	{ .verb = "unsubscribe", .callback = unsubscribe,  .info = "Unsubscribe to GNSS events" },
	{ }
};

/*
 * binder API description
 */
const struct afb_binding_v3 afbBindingV3 = {
	.api = "gps",
	.specification = "GNSS/GPS API",
	.verbs = binding_verbs,
	.init = init,
};
