/* 
 *   Copyright 2017 Konsulko Group
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <json-c/json.h>

#define AFB_BINDING_VERSION 2
#include <afb/afb-binding.h>

#include "media-manager.h"

static struct afb_event media_added_event;
static struct afb_event media_removed_event;

/*
 * @brief Subscribe for an event
 *
 * @param struct afb_req : an afb request structure
 *
 */
static void subscribe(struct afb_req request)
{
	const char *value = afb_req_value(request, "value");
	if(value) {
		if(!strcasecmp(value, "media_added")) {
			afb_req_subscribe(request, media_added_event);
		} else if(!strcasecmp(value, "media_removed")) {
			afb_req_subscribe(request, media_removed_event);
		} else {
			afb_req_fail(request, "failed", "Invalid event");
			return;
		}
	}
	afb_req_success(request, NULL, NULL);
}

/*
 * @brief Unsubscribe for an event
 *
 * @param struct afb_req : an afb request structure
 *
 */
static void unsubscribe(struct afb_req request)
{
	const char *value = afb_req_value(request, "value");
	if(value) {
		if(!strcasecmp(value, "media_added")) {
			afb_req_unsubscribe(request, media_added_event);
		} else if(!strcasecmp(value, "media_removed")) {
			afb_req_unsubscribe(request, media_removed_event);
		} else {
			afb_req_fail(request, "failed", "Invalid event");
			return;
		}
	}
	afb_req_success(request, NULL, NULL);
}

static json_object *new_json_object_from_device(GList *list)
{
    json_object *jarray = json_object_new_array();
    json_object *jresp = json_object_new_object();
    json_object *jstring = NULL;
    GList *l;

    for (l = list; l; l = l->next)
    {
        struct Media_Item *item = l->data;
        json_object *jdict = json_object_new_object();

        jstring = json_object_new_string(item->path);
        json_object_object_add(jdict, "path", jstring);

        jstring = json_object_new_string(lms_scan_types[item->type]);
        json_object_object_add(jdict, "type", jstring);

        if (item->metadata.title) {
            jstring = json_object_new_string(item->metadata.title);
            json_object_object_add(jdict, "title", jstring);
        }

        if (item->metadata.artist) {
            jstring = json_object_new_string(item->metadata.artist);
            json_object_object_add(jdict, "artist", jstring);
        }

        if (item->metadata.album) {
            jstring = json_object_new_string(item->metadata.album);
            json_object_object_add(jdict, "album", jstring);
        }

        if (item->metadata.genre) {
            jstring = json_object_new_string(item->metadata.genre);
            json_object_object_add(jdict, "genre", jstring);
        }

        if (item->metadata.duration) {
            json_object *jint = json_object_new_int(item->metadata.duration);
            json_object_object_add(jdict, "duration", jint);
        }

        json_object_array_add(jarray, jdict);
    }

    if (jstring == NULL)
        return NULL;

    json_object_object_add(jresp, "Media", jarray);

    return jresp;
}

static void media_results_get (struct afb_req request)
{
    GList *list = NULL;
    json_object *jresp = NULL;

    ListLock();
    list = media_lightmediascanner_scan(list, NULL, LMS_AUDIO_SCAN);
    list = media_lightmediascanner_scan(list, NULL, LMS_VIDEO_SCAN);
    if (list == NULL) {
        afb_req_fail(request, "failed", "media scan error");
        ListUnlock();
        return;
    }

    jresp = new_json_object_from_device(list);
    g_list_free(list);
    ListUnlock();

    if (jresp == NULL) {
        afb_req_fail(request, "failed", "media parsing error");
        return;
    }

    afb_req_success(request, jresp, "Media Results Displayed");
}

static void media_broadcast_device_added (GList *list)
{
    json_object *jresp = new_json_object_from_device(list);

    if (jresp != NULL) {
        afb_event_push(media_added_event, jresp);
    }
}

static void media_broadcast_device_removed (const char *obj_path)
{
    json_object *jresp = json_object_new_object();
    json_object *jstring = json_object_new_string(obj_path);

    json_object_object_add(jresp, "Path", jstring);

    afb_event_push(media_removed_event, jresp);
}

static const struct afb_verb_v2 binding_verbs[] = {
    { .verb = "media_result", .callback = media_results_get, .info = "Media scan result" },
    { .verb = "subscribe",    .callback = subscribe,         .info = "Subscribe for an event" },
    { .verb = "unsubscribe",  .callback = unsubscribe,       .info = "Unsubscribe for an event" },
    { }
};

static int preinit()
{
    Binding_RegisterCallback_t API_Callback;
    API_Callback.binding_device_added = media_broadcast_device_added;
    API_Callback.binding_device_removed = media_broadcast_device_removed;
    BindingAPIRegister(&API_Callback);

    return MediaPlayerManagerInit();
}

static int init()
{
    media_added_event = afb_daemon_make_event("media_added");
    media_removed_event = afb_daemon_make_event("media_removed");

    return 0;
}

const struct afb_binding_v2 afbBindingV2 = {
    .api = "mediascanner",
    .specification = "mediaplayer API",
    .preinit = preinit,
    .init = init,
    .verbs = binding_verbs,
};
