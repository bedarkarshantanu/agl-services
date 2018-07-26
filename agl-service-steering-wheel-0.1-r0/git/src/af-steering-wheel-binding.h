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

#ifndef _AF_WHEEL_BINDING_H_
#define _AF_WHEEL_BINDING_H_

#define AFB_BINDING_VERSION 2
#include <afb/afb-binding.h>

#include "prop_info.h"

#if !defined(NO_BINDING_VERBOSE_MACRO)
#define	ERRMSG(msg, ...) AFB_ERROR(msg,##__VA_ARGS__)
#define	WARNMSG(msg, ...) AFB_WARNING(msg,##__VA_ARGS__)
#define	DBGMSG(msg, ...) AFB_DEBUG(msg,##__VA_ARGS__)
#define	INFOMSG(msg, ...) AFB_INFO(msg,##__VA_ARGS__)
#define NOTICEMSG(msg, ...) AFB_NOTICE(msg,##__VA_ARGS__)
#endif

extern int notify_property_changed(struct prop_info_t *property_info);

#endif
