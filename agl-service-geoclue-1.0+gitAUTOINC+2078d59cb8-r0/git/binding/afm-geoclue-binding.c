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
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <geoclue.h>
#include <glib-object.h>
#include <json-c/json.h>

#define AFB_BINDING_VERSION 3
#include <afb/afb-binding.h>

static afb_event_t location_event;
static GClueSimple *simple;

/*
 * @latitude  - latitude in degrees
 * @longitude - longitude in degrees
 * @accuracy  - accuracy in meters
 * @altitude  - altitude in meters
 */

static json_object *populate_json(json_object *jresp, GClueLocation *location)
{
	json_object *value;
	double altitude = gclue_location_get_altitude(location);

	value = json_object_new_double(gclue_location_get_latitude(location));
	json_object_object_add(jresp, "latitude", value);

	value = json_object_new_double(gclue_location_get_longitude(location));
	json_object_object_add(jresp, "longitude", value);

	value = json_object_new_double(gclue_location_get_accuracy(location));
	json_object_object_add(jresp, "accuracy", value);

	if (altitude != -G_MAXDOUBLE) {
		value = json_object_new_double(altitude);
		json_object_object_add(jresp, "altitude", value);
	}

	return jresp;
}

/*
 * @heading - heading in degrees
 * @speed   - speed in meters per second
 */

static json_object *populate_velocity_json(json_object *jresp, GClueLocation *location)
{
	json_object *value;
	double heading = gclue_location_get_heading(location);
	double speed = gclue_location_get_speed(location);

	if (heading >= 0) {
		value = json_object_new_double(heading);
		json_object_object_add(jresp, "heading", value);
	}

	if (speed >= 0) {
		value = json_object_new_double(speed);
		json_object_object_add(jresp, "speed", value);
	}

	return jresp;
}

static void send_event(GClueSimple *simple)
{
	json_object *jresp;
	GClueLocation *location;

	location = gclue_simple_get_location(simple);
	jresp = json_object_new_object();

	populate_json(jresp, location);
	populate_velocity_json(jresp, location);
	afb_event_push(location_event, jresp);
}

static void get_data(afb_req_t request)
{
	json_object *jresp = json_object_new_object();
	GClueLocation *location;

	if (simple == NULL) {
		afb_req_fail(request, "failed", "No GeoClue instance available");
		return;
	}

	location = gclue_simple_get_location(simple);

	populate_json(jresp, location);
	afb_req_success(request, jresp, "GeoClue location data");
}

static void on_ready(GObject *source_object, GAsyncResult *res, gpointer ptr)
{
	GError *error = NULL;

	simple = gclue_simple_new_finish(res, &error);
	if (error != NULL) {
		AFB_ERROR("Failed to connect to GeoClue2 service: %s", error->message);
		abort();
	}

	send_event(simple);

	g_signal_connect(simple, "notify::location", G_CALLBACK(send_event), NULL);
}

static void *geoclue_loop_thread(void *ptr)
{
	gclue_simple_new("geoclue-binding", GCLUE_ACCURACY_LEVEL_EXACT, NULL, on_ready, NULL);

        g_main_loop_run(g_main_loop_new(NULL, FALSE));

	return NULL;
}

static int init(afb_api_t api)
{
	pthread_t thread_id;

	location_event = afb_daemon_make_event("location");

	return pthread_create(&thread_id, NULL, geoclue_loop_thread, NULL);
}

static void subscribe(afb_req_t request)
{
	const char *value = afb_req_value(request, "value");

	if (value && !strcasecmp(value, "location")) {
		afb_req_subscribe(request, location_event);
		afb_req_success(request, NULL, NULL);

		/*
		 * Sent out an initial event on subsciption since if location is
		 * static another event may not happen.
		 */
		if (simple) {
			send_event(simple);
		}
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

static const struct afb_verb_v3 binding_verbs[] = {
	{ .verb = "location",    .callback = get_data,     .info = "Get GeoClue coordinates" },
	{ .verb = "subscribe",   .callback = subscribe,    .info = "Subscribe to GeoClue events" },
	{ .verb = "unsubscribe", .callback = unsubscribe,  .info = "Unsubscribe to GeoClue events" },
	{ }
};

/*
 * binder API description
 */
const struct afb_binding_v3 afbBindingV3 = {
	.api = "geoclue",
	.specification = "GeoClue service API",
	.verbs = binding_verbs,
	.init = init,
};
