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
#define AFB_BINDING_VERSION 2
#include <afb/afb-binding.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <json-c/json.h>
#include <glib.h>
#include <pthread.h>
#include "hs-helper.h"
#include "hmi-debug.h"

#define COMMAND_EVENT_NUM 4
#define EVENT_SUBSCRIBE_ERROR_CODE 100

/* To Do hash table is better */
struct event{
    const char* name;
    struct afb_event* event;
    };

static struct event event_list[COMMAND_EVENT_NUM];

static struct afb_event ev_tap_shortcut;
static struct afb_event ev_on_screen_message;
static struct afb_event ev_on_screen_reply;
static struct afb_event ev_reserved;

static const char _error[] = "error";

static const char _application_name[] = "application_name";
static const char _display_message[] = "display_message";
static const char _reply_message[] = "reply_message";

/*
********** Method of HomeScreen Service (API) **********
*/

static void pingSample(struct afb_req request)
{
   static int pingcount = 0;
   afb_req_success_f(request, json_object_new_int(pingcount), "Ping count = %d", pingcount);
   HMI_NOTICE("homescreen-service","Verbosity macro at level notice invoked at ping invocation count = %d", pingcount);
   pingcount++;
}

/**
 * tap_shortcut notify for homescreen
 * When Shortcut area is tapped,  notify these applciations
  *
 * #### Parameters
 * Request key
 * - application_name   : application name
 *
 * #### Return
 * Nothing
 *
 */
static void tap_shortcut (struct afb_req request)
{
    HMI_NOTICE("homescreen-service","called.");

    int ret = 0;
    const char* value = afb_req_value(request, _application_name);
    if (value) {

      HMI_NOTICE("homescreen-service","request params = %s.", value);

      struct json_object* push_obj = json_object_new_object();
      hs_add_object_to_json_object_str( push_obj, 2,
      _application_name, value);
      afb_event_push(ev_tap_shortcut, push_obj);
    } else {
      afb_req_fail_f(request, "failed", "called %s, Unknown palameter", __FUNCTION__);
      return;
    }

  // response to HomeScreen
    struct json_object *res = json_object_new_object();
    hs_add_object_to_json_object_func(res, __FUNCTION__, 2,
      _error,  ret);
    afb_req_success(request, res, "afb_event_push event [tap_shortcut]");
}

/**
 * HomeScreen OnScreen message
 *
 * #### Parameters
 * Request key
 * - display_message   : message for display
 *
 * #### Return
 * Nothing
 *
 */
static void on_screen_message (struct afb_req request)
{
    HMI_NOTICE("homescreen-service","called.");

    int ret = 0;
    const char* value = afb_req_value(request, _display_message);
    if (value) {

      HMI_NOTICE("homescreen-service","request params = %s.", value);

      struct json_object* push_obj = json_object_new_object();
      hs_add_object_to_json_object_str( push_obj, 2,
      _display_message, value);
      afb_event_push(ev_on_screen_message, push_obj);
    } else {
      afb_req_fail_f(request, "failed", "called %s, Unknown palameter", __FUNCTION__);
      return;
    }

  // response to HomeScreen
    struct json_object *res = json_object_new_object();
    hs_add_object_to_json_object_func(res, __FUNCTION__, 2,
      _error,  ret);
    afb_req_success(request, res, "afb_event_push event [on_screen_message]");
}

/**
 * HomeScreen OnScreen Reply
 *
 * #### Parameters
 * Request key
 * - reply_message   : message for reply
 *
 * #### Return
 * Nothing
 *
 */
static void on_screen_reply (struct afb_req request)
{
    HMI_NOTICE("homescreen-service","called.");

    int ret = 0;
    const char* value = afb_req_value(request, _reply_message);
    if (value) {

      HMI_NOTICE("homescreen-service","request params = %s.", value);

      struct json_object* push_obj = json_object_new_object();
      hs_add_object_to_json_object_str( push_obj, 2,
      _reply_message, value);
      afb_event_push(ev_on_screen_reply, push_obj);
    } else {
      afb_req_fail_f(request, "failed", "called %s, Unknown palameter", __FUNCTION__);
      return;
    }

  // response to HomeScreen
    struct json_object *res = json_object_new_object();
    hs_add_object_to_json_object_func(res, __FUNCTION__, 2,
      _error,  ret);
    afb_req_success(request, res, "afb_event_push event [on_screen_reply]");
}

/**
 * Subscribe event
 *
 * #### Parameters
 *  - event  : Event name. Event list is written in libhomescreen.cpp
 *
 * #### Return
 * Nothing
 *
 * #### Note
 *
 */
static void subscribe(struct afb_req request)
{
    const char *value = afb_req_value(request, "event");
    HMI_NOTICE("homescreen-service","value is %s", value);
    int ret = 0;
    if(value) {
        int index = hs_search_event_name_index(value);
        if(index < 0)
        {
            HMI_NOTICE("homescreen-service","dedicated event doesn't exist");
            ret = EVENT_SUBSCRIBE_ERROR_CODE;
        }
        else
        {
            afb_req_subscribe(request, *event_list[index].event);
        }
    }
    else{
        HMI_NOTICE("homescreen-service","Please input event name");
        ret = EVENT_SUBSCRIBE_ERROR_CODE;
    }
    /*create response json object*/
    struct json_object *res = json_object_new_object();
    hs_add_object_to_json_object_func(res, __FUNCTION__, 2,
        _error, ret);
    afb_req_success_f(request, res, "homescreen binder subscribe event name [%s]", value);
}

/**
 * Unsubscribe event
 *
 * #### Parameters
 *  - event  : Event name. Event list is written in libhomescreen.cpp
 *
 * #### Return
 * Nothing
 *
 * #### Note
 *
 */
static void unsubscribe(struct afb_req request)
{
    const char *value = afb_req_value(request, "event");
    HMI_NOTICE("homescreen-service","value is %s", value);
    int ret = 0;
    if(value) {
        int index = hs_search_event_name_index(value);
        if(index < 0)
        {
            HMI_NOTICE("homescreen-service","dedicated event doesn't exist");
            ret = EVENT_SUBSCRIBE_ERROR_CODE;
        }
        else
        {
            afb_req_unsubscribe(request, *event_list[index].event);
        }
    }
    else{
        HMI_NOTICE("homescreen-service","Please input event name");
        ret = EVENT_SUBSCRIBE_ERROR_CODE;
    }
    /*create response json object*/
    struct json_object *res = json_object_new_object();
    hs_add_object_to_json_object_func(res, __FUNCTION__, 2,
        _error, ret);
    afb_req_success_f(request, res, "homescreen binder unsubscribe event name [%s]", value);
}

/*
 * array of the verbs exported to afb-daemon
 */
static const struct afb_verb_v2 verbs[]= {
    /* VERB'S NAME                    SESSION MANAGEMENT                FUNCTION TO CALL                     */
    { .verb = "ping",              .session = AFB_SESSION_NONE,    .callback = pingSample,        .auth = NULL },
    { .verb = "tap_shortcut",      .session = AFB_SESSION_NONE,    .callback = tap_shortcut,      .auth = NULL },
    { .verb = "on_screen_message", .session = AFB_SESSION_NONE,    .callback = on_screen_message, .auth = NULL },
    { .verb = "on_screen_reply",   .session = AFB_SESSION_NONE,    .callback = on_screen_reply,   .auth = NULL },
    { .verb = "subscribe",         .session = AFB_SESSION_NONE,    .callback = subscribe,         .auth = NULL },
    { .verb = "unsubscribe",       .session = AFB_SESSION_NONE,    .callback = unsubscribe,       .auth = NULL },
    {NULL } /* marker for end of the array */
};

static int preinit()
{
   HMI_NOTICE("homescreen-service","binding preinit (was register)");
   return 0;
}

static int init()
{
   HMI_NOTICE("homescreen-service","binding init");

   ev_tap_shortcut = afb_daemon_make_event(evlist[0]);
   ev_on_screen_message = afb_daemon_make_event(evlist[1]);
   ev_on_screen_reply = afb_daemon_make_event(evlist[2]);
   ev_reserved = afb_daemon_make_event(evlist[3]);

   event_list[0].name = evlist[0];
   event_list[0].event = &ev_tap_shortcut;

   event_list[1].name = evlist[1];
   event_list[1].event = &ev_on_screen_message;

   event_list[2].name = evlist[2];
   event_list[2].event = &ev_on_screen_reply;

   event_list[3].name = evlist[3];
   event_list[3].event = &ev_reserved;

   return 0;
}

static void onevent(const char *event, struct json_object *object)
{
   HMI_NOTICE("homescreen-service","on_event %s", event);
}

const struct afb_binding_v2 afbBindingV2 = {
    .api = "homescreen",
    .specification = NULL,
    .verbs = verbs,
    .preinit = preinit,
    .init = init,
    .onevent = onevent
};
