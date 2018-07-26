/*
 * Copyright (C) 2018 Konsulko Group
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
 */

#define _GNU_SOURCE
#include <errno.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <glib.h>
#include <gio/gio.h>
#include <curl/curl.h>
#include <json-c/json.h>

#define AFB_BINDING_VERSION 3
#include <afb/afb-binding.h>

#define OPENWEATHER_API_KEY     "a860fa437924aec3d0360cc749e25f0e"
#define OPENWEATHER_URL         "http://api.openweathermap.org/data/2.5/weather?lat=%.4f&lon=%.4f&units=imperial&APPID=%s"

struct {
	gchar *api_key;
	gchar *url;
	gchar buffer[1024];
} data;

static pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;
static afb_event_t weather_event;

// Forward declaration
gboolean update_weather_data(gpointer ptr);

static void *weather_loop_thread(void *ptr)
{
	g_timeout_add_seconds(900, update_weather_data, NULL);

	g_main_loop_run(g_main_loop_new(NULL, FALSE));

	return NULL;
}

static int init(afb_api_t api)
{
	pthread_t thread_id;
	json_object *response, *query;
	int ret;

	ret = afb_daemon_require_api("persistence", 1);
	if (ret < 0) {
		AFB_ERROR("Cannot request data persistence service");
		return ret;
	}

	query = json_object_new_object();
	json_object_object_add(query, "key", json_object_new_string("OPENWEATHERMAP_API_KEY"));

	ret = afb_service_call_sync("persistence", "read", query, &response, NULL, NULL);

	if (ret < 0) {
		data.api_key = g_strdup(OPENWEATHER_API_KEY);
		AFB_WARNING("Cannot get OPENWEATHERMAP_API_KEY from persistence storage, defaulting to %s", data.api_key);
	} else {
		json_object *jresp = NULL;

		json_object_object_get_ex(response, "response", &jresp);
		json_object_object_get_ex(jresp, "value", &jresp);

		data.api_key = g_strdup(json_object_get_string(jresp));

		AFB_NOTICE("OPENWEATHERMAP_API_KEY retrieved from persistence: %s", data.api_key);
	}

	json_object_put(response);

	ret = afb_daemon_require_api("geoclue", 1);
	if (ret < 0) {
		AFB_ERROR("Cannot request GeoClue service");
		return ret;
	}

	query = json_object_new_object();
	json_object_object_add(query, "value", json_object_new_string("location"));

	ret = afb_service_call_sync("geoclue", "subscribe", query, &response, NULL, NULL);
	json_object_put(response);

	if (ret < 0) {
		AFB_ERROR("Cannot subscribe to GeoClue service");
		return ret;
	}

	weather_event = afb_daemon_make_event("weather");

	return pthread_create(&thread_id, NULL, weather_loop_thread, NULL);
}

static size_t weather_cb(char *ptr, size_t size, size_t nmemb, void *usr)
{
	json_object *jresp = NULL;
	size_t realsize = size * nmemb;

	if (realsize > (sizeof(data.buffer) - 1))
		return -ENOMEM;

	pthread_rwlock_rdlock(&rwlock);

	memcpy(data.buffer, ptr, realsize);
	data.buffer[realsize] = '\0';

	jresp = json_tokener_parse(data.buffer);

	if (jresp)
		json_object_object_add(jresp, "url", json_object_new_string(data.url));

	pthread_rwlock_unlock(&rwlock);

	afb_event_push(weather_event, jresp);

	return realsize;
}

gboolean update_weather_data(gpointer ptr)
{
	CURL *ch;

	pthread_rwlock_rdlock(&rwlock);

	if (data.url == NULL) {
		pthread_rwlock_unlock(&rwlock);
		return TRUE;
	}

	ch = curl_easy_init();

	if (ch == NULL) {
		pthread_rwlock_unlock(&rwlock);
		return TRUE;
	}

	curl_easy_setopt(ch, CURLOPT_URL, data.url);
	curl_easy_setopt(ch, CURLOPT_WRITEFUNCTION, weather_cb);

	(void) curl_easy_perform(ch);
	curl_easy_cleanup(ch);

	pthread_rwlock_unlock(&rwlock);

	return TRUE;
}

static void onevent(afb_api_t api, const char *event, struct json_object *object)
{
	json_object *val = NULL;
	double latitude, longitude;
	int ret;

	if (strcasecmp(event, "geoclue/location") || data.api_key == NULL)
		return;

	ret = json_object_object_get_ex(object, "latitude", &val);
	if (!ret)
		return;
	latitude = json_object_get_double(val);

	ret = json_object_object_get_ex(object, "longitude", &val);
	if (!ret)
		return;
	longitude = json_object_get_double(val);
	ret = 0;

	pthread_rwlock_wrlock(&rwlock);

	if (data.url != NULL) {
		g_free(data.url);
		data.url = NULL;
		ret = -1;
	}

	data.url = g_strdup_printf(OPENWEATHER_URL, latitude, longitude, data.api_key);

	pthread_rwlock_unlock(&rwlock);

	// Initial Weather Data
	if (data.url != NULL && !ret)
		update_weather_data(NULL);
}

static void current_weather(afb_req_t request)
{
	json_object *jresp = NULL;

	pthread_rwlock_rdlock(&rwlock);

	jresp = json_tokener_parse(data.buffer);

	if (jresp)
		json_object_object_add(jresp, "url", json_object_new_string(data.url));

	pthread_rwlock_unlock(&rwlock);

	if (jresp == NULL) {
		afb_req_fail(request, "failed", "No weather data currently");
		return;
	}

	afb_req_success(request, jresp, "weather data");
}

static int validate_key(const char *value)
{
	int i;

	for (i = 0; i < strlen(value); i++)
		if (!isxdigit(value[i])) return -EINVAL;

	return 0;
}

static void api_key(afb_req_t request)
{
	const char *value = afb_req_value(request, "value");
	json_object *jresp = NULL;
	int ret;

	if (!value) {
		jresp = json_object_new_object();
		json_object_object_add(jresp, "api_key", json_object_new_string(data.api_key));
		afb_req_success(request, jresp, "application key");
		return;
	}

	if (strlen(value) != strlen(OPENWEATHER_API_KEY) || validate_key(value)) {
		afb_req_fail(request, "failed", "Invalid OpenWeatherMap API key");
		return;
	}

	jresp = json_object_new_object();
	json_object_object_add(jresp, "key", json_object_new_string("OPENWEATHERMAP_API_KEY"));
	json_object_object_add(jresp, "value", json_object_new_string(value));

	ret = afb_service_call_sync("persistence", "update", jresp, NULL, NULL, NULL);
	if (ret < 0) {
		afb_req_fail(request, "failed", "Couldn't store API key");
		return;
	}

	if (data.api_key != NULL) {
		g_free(data.api_key);
		data.api_key = g_strdup(value);
	}

	afb_req_success(request, jresp, NULL);
}

static void subscribe(afb_req_t request)
{
	const char *value = afb_req_value(request, "value");

	if (value && !strcasecmp(value, "weather")) {
		json_object *jresp = NULL;

		afb_req_subscribe(request, weather_event);
		afb_req_success(request, NULL, NULL);

		jresp = json_tokener_parse(data.buffer);
		if (jresp) {
			json_object_object_add(jresp, "url", json_object_new_string(data.url));
			afb_event_push(weather_event, jresp);
		}
		return;
	}

	afb_req_fail(request, "failed", "Invalid event");
}

static void unsubscribe(afb_req_t request)
{
	const char *value = afb_req_value(request, "value");

	if (value && !strcasecmp(value, "weather")) {
		afb_req_unsubscribe(request, weather_event);
		afb_req_success(request, NULL, NULL);
		return;
	}

	afb_req_fail(request, "failed", "Invalid event");
}

static const struct afb_verb_v3 binding_verbs[] = {
	{ .verb = "current_weather", .callback = current_weather, .info = "Current weather data" },
	{ .verb = "api_key",     .callback = api_key,      .info = "Get/set OpenWeatherMap API key" },
	{ .verb = "subscribe",   .callback = subscribe,    .info = "Subscribe to weather events" },
	{ .verb = "unsubscribe", .callback = unsubscribe,  .info = "Unsubscribe to weather events" },
	{ }
};

/*
 * binder API description
 */
const struct afb_binding_v3 afbBindingV3 = {
	.api		= "weather",
	.specification  = "Weather service API",
	.verbs		= binding_verbs,
	.init 		= init,
	.onevent	= onevent,
};
