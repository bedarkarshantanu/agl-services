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

#ifndef _WHEEL_JSON_H_
#define _WHEEL_JSON_H_

#include <fcntl.h>
#include <json-c/json.h>

extern int wheel_define_init(const char *fname);
extern int wheel_gear_para_init(const char *fname);
extern struct json_object *property2json(struct prop_info_t *property);

#endif
