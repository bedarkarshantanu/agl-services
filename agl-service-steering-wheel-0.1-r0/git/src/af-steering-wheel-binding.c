/*
 * Copyright (c) 2017 TOYOTA MOTOR CORPORATION
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

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <netdb.h>
#include <fcntl.h>
#include <math.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <json-c/json.h>

#include <systemd/sd-event.h>

#include "af-steering-wheel-binding.h"
#include "bind_event.h"
#include "steering_wheel_json.h"
#include "prop_search.h"
#include "js_raw.h"

#define ENABLE_EVENT_DROP

struct wheel_conf
{
	char *devname;
};

int notify_property_changed(struct prop_info_t *property_info)
{
	struct bind_event_t *event;

	//NOTICEMSG("notify_property_changed %s", property_info->name);
	event =  get_event(property_info->name);
	if (event == NULL)
	{
			DBGMSG("<IGNORE event>: event(\"%s\") is not subscribed", property_info->name);
			return 1;
	}

	if (afb_event_is_valid(event->event))
	{
		int recved_client;

		recved_client = afb_event_push(event->event, property2json(property_info));
		if(recved_client < 1)
		{
			if(recved_client == 0)
			{
				DBGMSG("event(\"%s\") is no client.", property_info->name);
#ifdef ENABLE_EVENT_DROP
				/* Drop event */
				remove_event(property_info->name);
#endif
			}
			else
			{
				ERRMSG("count of clients that received the event : %d",recved_client);
			}

			return 1;
		} /* else  (1 <= recved_lient)
		   *	NORMAL
		   */
	}
	else
	{
		ERRMSG("event(\"%s\") is invalid. don't adb_event_push()", property_info->name);
		return 1;
	}

	return 0;
}

static int	connection(struct wheel_conf *conf)
{
	int js;
	int ret;
	sd_event_source *source;

	js = js_open(conf->devname);
	if (js < 0)
	{
		js_close(js);
		ERRMSG("can't connect to joy stick, the event loop failed");
		return 0;
	}

	ret = sd_event_add_io(
			    afb_daemon_get_event_loop(),
				&source,
				js,
				EPOLLIN,
				on_event, NULL);
	if (ret < 0)
	{
		js_close(js);
		ERRMSG("can't add event, the event loop failed");
		return 0;
	}

	NOTICEMSG("connect to JS(%s), event loop started",conf->devname);

	return 0;
}

/***************************************************************************************/
/**                                                                                   **/
/**                                                                                   **/
/**       SECTION: BINDING VERBS IMPLEMENTATION                                       **/
/**                                                                                   **/
/**                                                                                   **/
/***************************************************************************************/
/**
 * get method.
 */
static void get(struct afb_req req)
{

}

///**
// * set method.
// */
//static void set(struct afb_req req)
//{
//
//}

/**
 * list method.
 */
static struct json_object *tmplists;

static void _listup_properties(struct prop_info_t *property_info)
{
	if (property_info->name == NULL)
	{
		return;
	}
	else
	{
		json_object_array_add(tmplists, json_object_new_string(property_info->name));
	}
}

static void list(struct afb_req req)
{
	if (tmplists != NULL)
	{
		json_object_put(tmplists);
	}
	tmplists = json_object_new_array();
	if (tmplists == NULL)
	{
		ERRMSG("not enough memory");
		afb_req_fail(req,"not enough memory", NULL);
		return;
	}
	(void)canid_walker(_listup_properties);
	afb_req_success(req, tmplists, NULL);
}

/**
 * subscribe method.
 */
static int subscribe_all(struct afb_req req, struct json_object *apply);

static int  subscribe_signal(struct afb_req req, const char *name, struct json_object *apply)
{
	struct bind_event_t *event;

	if(strcmp(name, "*") == 0)
	{
		return subscribe_all(req, apply);
	}

	event = get_event(name);
	if (event == NULL)
	{
		event = register_event(name);
		if (event == NULL)
		{
			ERRMSG("register event failed: %s", name);
			return 1;
		}
	}
	else
	{
		DBGMSG("subscribe event: \"%s\" alrady exist", name);
	}

	if (afb_req_subscribe(req, event->event) != 0)
	{
		ERRMSG("afb_req_subscrive() is failed.");
		return 1;
	}

	json_object_object_add(apply, event->name, property2json(event->raw_info));

	return 0;
}

static int subscribe_all(struct afb_req req, struct json_object *apply)
{
	int i;
	int nProp;
	const char **list = getSupportPropertiesList(&nProp);
	int err = 0;
	for(i = 0; i < nProp; i++)
		err += subscribe_signal(req, list[i], apply);

	return err;
}

static void subscribe(struct afb_req req)
{
	struct json_object *apply;
	int err = 0;
	const char *event;

	event = afb_req_value(req, "event");
	if (event == NULL)
	{
		ERRMSG("Unknwon subscrive event args");
		afb_req_fail(req, "error", NULL);
		return;
	}
	apply = json_object_new_object();
	err = subscribe_signal(req, event, apply);

	if (!err)
	{
		afb_req_success(req, apply, NULL);
	}
	else
	{
		json_object_put(apply);
		afb_req_fail(req, "error", NULL);
	}
}

/**
 * unsubscribe method.
 */

static int unsubscribe_all(struct afb_req req);

static int unsubscribe_signal(struct afb_req req, const char *name)
{
	struct bind_event_t *event;
	if(strcmp(name, "*") == 0)
	{
		return unsubscribe_all(req);
	}

	event = get_event(name);
	if (event == NULL)
	{
		ERRMSG("not subscribed event \"%s\"", name);
		return 0; /* Alrady unsubscribed */
	}
	if (afb_req_unsubscribe(req, event->event) != 0)
	{
		ERRMSG("afb_req_subscrive() is failed.");
		return 1;
	}

	return 0;
}

static int unsubscribe_all(struct afb_req req)
{
	int i;
	int nProp;
	const char **list = getSupportPropertiesList(&nProp);
	int err = 0;
	for(i = 0; i < nProp; i++)
	{
		err += unsubscribe_signal(req, list[i]);
	}

	return err;
}

static void unsubscribe(struct afb_req req)
{
	struct json_object *args, *val;
	int n,i;
	int err = 0;

	args = afb_req_json(req);
	if ((args == NULL) || !json_object_object_get_ex(args, "event", &val))
	{
		err = unsubscribe_all(req);
	}
	else if (json_object_get_type(val) != json_type_array)
	{
		err = unsubscribe_signal(req, json_object_get_string(val));
	}
	else
	{
		struct json_object *ent;
		n = json_object_array_length(val);
		for (i = 0; i < n; i++)
		{
			ent = json_object_array_get_idx(val,i);
			err += unsubscribe_signal(req, json_object_get_string(ent));
		}
	}

	if (!err)
		afb_req_success(req, NULL, NULL);
	else
		afb_req_fail(req, "error", NULL);
}

/*
 * parse configuration file (steering_wheel.json)
 */
static int init_conf(int fd_conf, struct wheel_conf *conf)
{
#define CONF_FILE_MAX (1024)
	char filebuf[CONF_FILE_MAX];
	json_object *jobj;

	memset(filebuf,0, sizeof(filebuf));
	FILE *fp = fdopen(fd_conf,"r");
	if (fp == NULL)
	{
		ERRMSG("canno read configuration file(steering_wheel.json)");
		return -1;
	}

	fread(filebuf, 1, sizeof(filebuf), fp);
	fclose(fp);

	jobj = json_tokener_parse(filebuf);
	if (jobj == NULL)
	{
		ERRMSG("json: Invalid steering_wheel.json format");
		return -1;
	}
	json_object_object_foreach(jobj, key, val)
	{
		if (strcmp(key,"dev_name") == 0)
		{
			conf->devname = strdup(json_object_get_string(val));
		}
		else if (strcmp(key,"wheel_map") == 0)
		{
			wheel_define_init(json_object_get_string(val));
		}
		else if (strcmp(key,"gear_para") == 0)
		{
			wheel_gear_para_init(json_object_get_string(val));
		}
	}
	json_object_put(jobj);

	return 0;
}

static int init()
{
	NOTICEMSG("init");

	int fd_conf;
	struct wheel_conf conf;
   	fd_conf = afb_daemon_rootdir_open_locale("steering_wheel.json", O_RDONLY, NULL);
	if (fd_conf < 0)
	{
		ERRMSG("wheel configuration (steering_wheel.json) is not access");
		return -1;
	}
	if (init_conf(fd_conf, &conf))
	{
		return -1;
	}
	return connection(&conf);
}

/*
 * array of the verbs exported to afb-daemon
 */
static const struct afb_verb_v2 binding_verbs[] =
{
  /* VERB'S NAME            SESSION MANAGEMENT          FUNCTION TO CALL         Authorization */
  { .verb= "get",          .session= AFB_SESSION_NONE, .callback= get,          .info = "get", .auth = NULL },
//  { .name= "set",          .session= AFB_SESSION_NONE, .callback= set,        .auth = NULL },
  { .verb= "list",         .session= AFB_SESSION_NONE, .callback= list,         .info = "list", .auth = NULL },
  { .verb= "subscribe",    .session= AFB_SESSION_NONE, .callback= subscribe,    .info = "subscribe", .auth = NULL },
  { .verb= "unsubscribe",  .session= AFB_SESSION_NONE, .callback= unsubscribe,  .info = "unsubscribe", .auth = NULL },
  { .verb= NULL } /* marker for end of the array */
};

/*
 * description of the binding for afb-daemon
 */
const struct afb_binding_v2 afbBindingV2 =
{
	.api = "steering-wheel",
	.specification = NULL,
    .verbs = binding_verbs,			 /* the array describing the verbs of the API */
    .preinit = NULL,
    .init = init,
    .onevent = NULL,
    .noconcurrency = 1
};
