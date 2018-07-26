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
 *
 * TODO: add support for geoclue binding location data
 */

#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <glib.h>
#include <glib-object.h>
#include <json-c/json.h>

#define AFB_BINDING_VERSION 3
#include <afb/afb-binding.h>

#define ARRAY_SIZE(x)	(sizeof(x)/sizeof(void *))
#define SECS_TO_USECS(x) (x * 1000000)
#define USECS_TO_SECS(x) (x / 1000000)

static afb_event_t fence_event;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static GList *fences = NULL;

static long dwell_transition_in_usecs = SECS_TO_USECS(20);

/*
 * @name      - geofence name
 * @triggered - geofence is currently triggered
 * @timestamp - timestamp of last geofence 'entered' transition
 * @bbox      - struct of bounding box coordinates
 */
struct geofence {
	gchar *name;
	bool triggered;
	gint64 timestamp;
	struct {
		double min_latitude, max_latitude;
		double min_longitude, max_longitude;
	} bbox;
};

const char *points[] = {
	"min_latitude",
	"max_latitude",
	"min_longitude",
	"max_longitude",
};

static void subscribe(afb_req_t request)
{
	const char *value = afb_req_value(request, "value");

	if (value && !strcasecmp(value, "fence")) {
		afb_req_subscribe(request, fence_event);
		afb_req_success(request, NULL, NULL);
		return;
	}

	afb_req_fail(request, "failed", "invalid event");
}

static void unsubscribe(afb_req_t request)
{
	const char *value = afb_req_value(request, "value");

	if (value && !strcasecmp(value, "fence")) {
		afb_req_unsubscribe(request, fence_event);
		afb_req_success(request, NULL, NULL);
		return;
	}

	afb_req_fail(request, "failed", "invalid event");
}

static inline bool within_bounding_box(double latitude, double longitude,
				       struct geofence *fence)
{
	if (latitude < fence->bbox.min_latitude)
		return false;

	if (latitude > fence->bbox.max_latitude)
		return false;

	if (longitude < fence->bbox.min_longitude)
		return false;

	if (longitude > fence->bbox.max_longitude)
		return false;

	return true;
}

static int parse_bounding_box(const char *data, struct geofence *fence)
{
	json_object *jquery = json_tokener_parse(data);
	json_object *val = NULL;
	double *bbox = (double *) &fence->bbox;
	int ret = -EINVAL, i;

	if (jquery == NULL)
		return -EINVAL;

	for (i = 0; i < ARRAY_SIZE(points); i++) {
		ret = json_object_object_get_ex(jquery, points[i], &val);
		if (!ret) {
			ret = -EINVAL;
			break;
		}

		*bbox++ = json_object_get_double(val);
	}

	json_object_put(jquery);

	if (fence->bbox.min_latitude > fence->bbox.max_latitude)
		return -EINVAL;

	if (fence->bbox.min_longitude > fence->bbox.max_longitude)
		return -EINVAL;

	return ret;
}

static void add_fence(afb_req_t request)
{
	const char *name = afb_req_value(request, "name");
	const char *bbox = afb_req_value(request, "bbox");
	struct geofence *fence;
	GList *l;
	int ret;

	if (!name) {
		afb_req_fail(request, "failed", "invalid name parameter");
		return;
	}

	if (!bbox) {
		afb_req_fail(request, "failed", "no bbox parameter found");
		return;
	}

	fence = g_try_malloc0(sizeof(struct geofence));
	if (fence == NULL) {
		afb_req_fail(request, "failed", "cannot allocate memory");
		return;
	}

	ret = parse_bounding_box(bbox, fence);
	if (ret < 0) {
		afb_req_fail(request, "failed", "invalid bounding box");
		g_free(fence);
		return;
	}

	pthread_mutex_lock(&mutex);

	for (l = fences; l; l = l->next) {
		struct geofence *g = l->data;

		if (g_strcmp0(g->name, name) == 0) {
			g_free(fence);
			pthread_mutex_unlock(&mutex);

			afb_req_fail(request, "failed", "fence with name exists");
			return;
		}
	}

	fence->name = g_strdup(name);
	fence->timestamp = -1;
	fences = g_list_append(fences, fence);

        pthread_mutex_unlock(&mutex);

	afb_req_success(request, NULL, NULL);
}

static void remove_fence(afb_req_t request)
{
	const char *name = afb_req_value(request, "name");
	GList *l;

	if (!name) {
		afb_req_fail(request, "failed", "invalid id parameter");
		return;
	}

	pthread_mutex_lock(&mutex);

	for (l = fences; l; l = l->next) {
		struct geofence *g = l->data;

		if (g_strcmp0(g->name, name) == 0) {
			fences = g_list_remove(fences, g);
			g_free(g->name);
			g_free(g);

			pthread_mutex_unlock(&mutex);

			afb_req_success(request, NULL, "removed fence");
			return;
		}
	}

	pthread_mutex_unlock(&mutex);

	afb_req_fail(request, "failed", "fence not found for removal");
}

static void list_fences(afb_req_t request)
{
	json_object *jresp = json_object_new_object();
	GList *l;

	pthread_mutex_lock(&mutex);

	for (l = fences; l; l = l->next)
	{
		json_object *json_bbox = json_object_new_object();
		json_object *item = json_object_new_object();
		struct geofence *g = l->data;
		double *bbox = (double *) &g->bbox;
		int i;

		for (i = 0; i < ARRAY_SIZE(points); i++) {
			json_object_object_add(json_bbox, points[i],
				       json_object_new_double(*bbox++));
		}

		json_object_object_add(item, "within",
			json_object_new_boolean(g->triggered));
		json_object_object_add(item, "dwell",
			json_object_new_boolean(g->triggered && g->timestamp == -1));
		json_object_object_add(item, "bbox", json_bbox);
		json_object_object_add(jresp, g->name, item);
	}

	pthread_mutex_unlock(&mutex);

	afb_req_success(request, jresp, "list of geofences");
}

static int init(afb_api_t api)
{
	json_object *response, *query;
	int ret;

	ret = afb_daemon_require_api("gps", 1);
	if (ret < 0) {
		AFB_ERROR("Cannot request gps service");
		return ret;
	}

	query = json_object_new_object();
	json_object_object_add(query, "value", json_object_new_string("location"));

	ret = afb_service_call_sync("gps", "subscribe", query, &response, NULL, NULL);
	json_object_put(response);

	if (ret < 0) {
		AFB_ERROR("Cannot subscribe to gps service");
		return ret;
	}

	fence_event = afb_daemon_make_event("fence");

	return 0;
}

static inline bool dwell_transition_state(struct geofence *g)
{
	// zero dwell_transition_in_usecs means disabled
	if (dwell_transition_in_usecs == 0)
		return false;

	if (g->timestamp == -1)
		return false;

	if ((g_get_monotonic_time() - g->timestamp) < dwell_transition_in_usecs)
		return false;

	return true;
}

static void dwell_transition(afb_req_t request)
{
	const char *value = afb_req_value(request, "value");
	json_object *jresp = NULL;

	pthread_mutex_lock(&mutex);

	if (value) {
		long seconds = strtol(value, NULL, 10);
		if (seconds < 0) {
			afb_req_fail(request, "failed", "invalid input");
			pthread_mutex_unlock(&mutex);
			return;
		}

		dwell_transition_in_usecs = SECS_TO_USECS(seconds);
	}

	jresp = json_object_new_object();
	json_object_object_add(jresp, "seconds",
		json_object_new_int64(USECS_TO_SECS(dwell_transition_in_usecs)));

	pthread_mutex_unlock(&mutex);

	afb_req_success(request, jresp,
			"loitering time in seconds to enter dwell transition");
}

static void onevent(afb_api_t api, const char *event, struct json_object *object)
{
	json_object *val = NULL;
	double latitude, longitude;
	GList *l;
	int ret;

	if (g_strcmp0(event, "gps/location"))
		return;

	ret = json_object_object_get_ex(object, "latitude", &val);
	if (!ret)
		return;
	latitude = json_object_get_double(val);

	ret = json_object_object_get_ex(object, "longitude", &val);
	if (!ret)
		return;
	longitude = json_object_get_double(val);

	pthread_mutex_lock(&mutex);

	for (l = fences; l; l = l->next) {
		struct geofence *g = (struct geofence *) l->data;
		struct json_object *jresp;
		bool current = within_bounding_box(latitude, longitude, g);

		if (current == g->triggered && !dwell_transition_state(g))
			continue;

		jresp = json_object_new_object();

		json_object_object_add(jresp, "name",
			json_object_new_string(g->name));

		if (current && g->triggered) {
			json_object_object_add(jresp, "state",
				json_object_new_string("dwell"));
			g->timestamp = -1;
		} else if (current) {
			json_object_object_add(jresp, "state",
				json_object_new_string("entered"));
			g->timestamp = g_get_monotonic_time();
		} else {
			json_object_object_add(jresp, "state",
				json_object_new_string("exited"));
			g->timestamp = -1;
		}

		g->triggered = current;

		afb_event_push(fence_event, jresp);
	}

	pthread_mutex_unlock(&mutex);
}

static const struct afb_verb_v3 binding_verbs[] = {
	{ .verb = "subscribe",    .callback = subscribe,    .info = "Subscribe to geofence events" },
	{ .verb = "unsubscribe",  .callback = unsubscribe,  .info = "Unsubscribe to geofence events" },
	{ .verb = "add_fence",    .callback = add_fence,    .info = "Add geofence" },
	{ .verb = "remove_fence", .callback = remove_fence, .info = "Remove geofence" },
	{ .verb = "list_fences",  .callback = list_fences,  .info = "List all curent geofences" },
	{ .verb = "dwell_transition",   .callback = dwell_transition,   .info = "Set/get dwell transition delay" },
	{ }
};

/*
 * binder API description
 */
const struct afb_binding_v3 afbBindingV3 = {
	.api		= "geofence",
	.specification	= "Geofence service API",
	.verbs		= binding_verbs,
	.init		= init,
	.onevent	= onevent,
};
