/*
 * Copyright (C) 2015, 2016, 2017 "IoT.bzh"
 * Author: José Bollo <jose.bollo@iot.bzh>
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

#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <json-c/json.h>

#define AFB_BINDING_VERSION 2
#include <afb/afb-binding.h>

#include "agl-forgerock.h"

#ifndef NULL
#define NULL 0
#endif

static struct afb_event event;

static struct json_object *current_identity;

static const char default_vin[] = "4T1BF1FK5GU260429";
static char *vin;

/***** configuration ********************************************/

static struct json_object *readjson(int fd)
{
	char *buffer;
	struct stat s;
	struct json_object *result = NULL;
	int rc;

	rc = fstat(fd, &s);
	if (rc == 0 && S_ISREG(s.st_mode)) {
		buffer = alloca((size_t)(s.st_size)+1);
		if (read(fd, buffer, (size_t)s.st_size) == (ssize_t)s.st_size) {
			buffer[s.st_size] = 0;
			//AFB_NOTICE("Config file: %s", buffer);
			result = json_tokener_parse(buffer);
			//if (!result)
			//{
			//	AFB_ERROR("Config file is not a valid JSON: %s", json_tokener_error_desc(json_tokener_get_error(NULL)));
			//}
		}
	}
	close(fd);

	return result;
}

static struct json_object *get_global_config(const char *name, const char *locale)
{
	int fd = afb_daemon_rootdir_open_locale(name, O_RDONLY, locale);
	if (fd < 0) AFB_ERROR("Config file not found: %s", name);
	return fd < 0 ? NULL : readjson(fd);
}

static struct json_object *get_local_config(const char *name)
{
	int fd = openat(AT_FDCWD, name, O_RDONLY, 0);
	return fd < 0 ? NULL : readjson(fd);
}

static void confsetstr(struct json_object *conf, const char *name, char **value, const char *def)
{
	struct json_object *v;
	const char *s;
	char *p;

	s = conf && json_object_object_get_ex(conf, name, &v) ? json_object_get_string(v) : def;
	p = *value;
	if (s && p != s) {
		*value = strdup(s);
		free(p);
	}
}

static void setconfig(struct json_object *conf)
{
	if (conf) {
		confsetstr(conf, "vin", &vin, vin ? : default_vin);
		agl_forgerock_setconfig(conf);
	}
}

static void readconfig()
{
	setconfig(get_global_config("etc/config.json", NULL));
	setconfig(get_local_config("/etc/agl/identity-agent-config.json"));
	setconfig(get_local_config("config.json"));
}

/****************************************************************/

static struct json_object *make_event_object(const char *name, const char *id, const char *nick)
{
	struct json_object *object = json_object_new_object();

	/* TODO: errors */
	json_object_object_add(object, "eventName", json_object_new_string(name));
	json_object_object_add(object, "accountid", json_object_new_string(id));
	if (nick)
		json_object_object_add(object, "nickname", json_object_new_string(nick));
	return object;
}

static int send_event_object(const char *name, const char *id, const char *nick)
{
	return afb_event_push(event, make_event_object(name, id, nick));
}

static void do_login(struct json_object *desc)
{
	if (current_identity == NULL && desc == NULL) return; // Switching from NULL to NULL -> do nothing
	if (current_identity && desc)
	{
		const char* a = json_object_to_json_string(current_identity);
		const char* b = json_object_to_json_string(desc);
		if (strcmp(a, b) == 0)
		{
			AFB_NOTICE("Reloging of the same user.");
			return; // Switching from one user to the same user -> do nothing
		}
	}

	struct json_object *object;

	/* switching the user */
	AFB_INFO("Switching to user %s", desc ? json_object_to_json_string(desc) : "null");
	object = current_identity;
	current_identity = json_object_get(desc);
	json_object_put(object);

	if (!json_object_object_get_ex(desc, "name", &object))
		object = 0;
	send_event_object("login", !object ? "null" : json_object_get_string(object)? : "?", 0);
}

static void do_logout()
{
	struct json_object *object;

	AFB_INFO("Switching to no user");
	object = current_identity;
	current_identity = 0;
	json_object_put(object);

	send_event_object("logout", "null", 0);
}

static void on_forgerock_data(struct json_object *data, const char *error)
{
	if (error) {
		AFB_ERROR("Can't get data: %s", error);
	} else {
		do_login(data);
	}
}

/****************************************************************/

static void subscribe (struct afb_req request)
{
	int rc;

	rc = afb_req_subscribe(request, event);
	if (rc < 0)
		afb_req_fail(request, "failed", "subscribtion failed");
	else
		afb_req_success(request, NULL, NULL);
}

static void unsubscribe (struct afb_req request)
{
	afb_req_unsubscribe(request, event);
	afb_req_success(request, NULL, NULL);
}

static void logout (struct afb_req request)
{
	do_logout();
	afb_req_success(request, NULL, NULL);
}

static void fake_login (struct afb_req request)
{
	struct json_object *desc = afb_req_json(request);
	do_logout();
	if (desc)
		do_login(desc);
	afb_req_success(request, NULL, NULL);
}

static void get (struct afb_req request)
{
	afb_req_success(request, json_object_get(current_identity), NULL);
}

static void on_nfc_subscribed(void *closure, int status, struct json_object *result)
{
	if(status)
		AFB_ERROR("Failed to subscribe to nfc events.");
}

static void on_nfc_started(void *closure, int status, struct json_object *result)
{
	if (!status) {
		afb_service_call("nfc", "subscribe", NULL, on_nfc_subscribed, NULL);
	}
	else
		AFB_ERROR("Failed to start nfc polling.");
}

static int service_init()
{
	agl_forgerock_setcb(on_forgerock_data);
	event = afb_daemon_make_event("event");
	if (!afb_event_is_valid(event))
		return -1;

	readconfig();

	if (afb_daemon_require_api("nfc", 1))
		return -1;

	if (afb_daemon_require_api("persistence", 1))
		return -1;

	afb_service_call("nfc", "start", NULL, on_nfc_started, NULL);

	return 0;
}

static void on_nfc_target_add(struct json_object *object)
{
	struct json_object * json_uid;
	const char *uid;

	if (json_object_object_get_ex(object, "UID", &json_uid))
	{
		uid = json_object_get_string(json_uid);
		AFB_NOTICE("nfc tag detected, call forgerock with vincode=%s and key=%s", vin ? vin : default_vin, uid);
		send_event_object("incoming", uid, uid);
		agl_forgerock_download_request(vin ? vin : default_vin, "nfc", uid);
	}
	else AFB_ERROR("nfc target add event is received but no UID found: %s", json_object_to_json_string(object));
}

static void onevent(const char *event, struct json_object *object)
{
	AFB_NOTICE("Received event: %s", event);
	if (!strcmp("nfc/on-nfc-target-add", event))
	{
		on_nfc_target_add(object);
		return;
	}
	AFB_WARNING("Unhandled event: %s", event);
}

static void fake_auth(struct afb_req req)
{
	struct json_object* req_object;
	struct json_object* kind_object;
	struct json_object* key_object;

	req_object = afb_req_json(req);

	if (!json_object_object_get_ex(req_object, "kind", &kind_object))
	{
		afb_req_fail(req, "Missing arg: kind", NULL);
		return;
	}

	if (!json_object_object_get_ex(req_object, "key", &key_object))
	{
		afb_req_fail(req, "Missing arg: key", NULL);
		return;
	}

	const char* kind = json_object_get_string(kind_object);
	const char* key = json_object_get_string(key_object);

	send_event_object("incoming", key, key);
	agl_forgerock_download_request(vin ? vin : default_vin, kind, key);

	afb_req_success(req, NULL, "fake auth success!");
}

// NOTE: this sample does not use session to keep test a basic as possible
//       in real application most APIs should be protected with AFB_SESSION_CHECK
static const struct afb_verb_v2 verbs[]=
{
  {"subscribe"  , subscribe    , NULL, "subscribe to events"     , AFB_SESSION_NONE },
  {"unsubscribe", unsubscribe  , NULL, "unsubscribe to events"   , AFB_SESSION_NONE },
  {"fake-login" , fake_login   , NULL, "fake a login"            , AFB_SESSION_NONE },
  {"logout"     , logout       , NULL, "log the current user out", AFB_SESSION_NONE },
  {"get"        , get          , NULL, "get data"                , AFB_SESSION_NONE },
  {"fake-auth"  , fake_auth    , NULL, "fake an authentication"  , AFB_SESSION_NONE },
  {NULL}
};

const struct afb_binding_v2 afbBindingV2 =
{
	.api = "identity",
	.specification = NULL,
	.info = "AGL identity service",
	.verbs = verbs,
	.preinit = NULL,
	.init = service_init,
	.onevent = onevent,
	.noconcurrency = 0
};

/* vim: set colorcolumn=80: */
