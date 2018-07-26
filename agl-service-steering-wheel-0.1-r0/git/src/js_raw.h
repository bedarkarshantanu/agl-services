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

#ifndef _JS_RAW_H_
#define _JS_RAW_H_

#include <sys/time.h>
#include <systemd/sd-event.h>

extern int on_event(sd_event_source *s, int fd, uint32_t revents, void *userdata);
extern int  js_open(const char *devname);
extern void js_close(int js);

#endif
