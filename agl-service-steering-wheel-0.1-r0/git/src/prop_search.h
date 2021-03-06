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

#ifndef _PROP_SEARCH_H_
#define _PROP_SEARCH_H_

#include "prop_info.h"

extern int addAllPropDict(struct wheel_info_t *wheel_info);
extern struct prop_info_t * getProperty_dict(const char *propertyName);
const char **getSupportPropertiesList(int *nEntry);
extern void canid_walker(void (*walker_action)(struct prop_info_t *));

#endif
