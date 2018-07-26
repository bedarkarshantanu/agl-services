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

#ifndef HOMESCREEN_HELPER_H
#define HOMESCREEN_HELPER_H
#define AFB_BINDING_VERSION 2
#include <afb/afb-binding.h>
#include <stdint.h>
#include <glib.h>
#include <errno.h>

typedef enum REQ_ERROR
{
  REQ_FAIL = -1,
  REQ_OK=0,
  NOT_NUMBER,
  OUT_RANGE
}REQ_ERROR;

static const char* evlist[] = {
    "tap_shortcut",
    "on_screen_message",
    "on_screen_reply",
    "reserved"
  };

REQ_ERROR get_value_uint16(const struct afb_req request, const char *source, uint16_t *out_id);
REQ_ERROR get_value_int16(const struct afb_req request, const char *source, int16_t *out_id);
REQ_ERROR get_value_int32(const struct afb_req request, const char *source, int32_t *out_id);
void hs_add_object_to_json_object(struct json_object* j_obj, int count, ...);
void hs_add_object_to_json_object_str(struct json_object* j_obj, int count, ...);
void hs_add_object_to_json_object_func(struct json_object* j_obj, const char* verb_name, int count, ...);
int hs_search_event_name_index(const char* value);

#endif /*HOMESCREEN_HELPER_H*/
