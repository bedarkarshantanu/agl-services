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
#include <stdlib.h>
#include <string.h>
#include <search.h>

#include "af-steering-wheel-binding.h"
#include "bind_event.h"
#include "prop_search.h"

static  void *bind_event_root = NULL;

static int compare(const void *pa, const void *pb)
{
	struct bind_event_t *a = (struct bind_event_t *)pa;
	struct bind_event_t *b = (struct bind_event_t *)pb;
	return strcmp(a->name, b->name);
}

struct bind_event_t *get_event(const char *name)
{
	struct bind_event_t key;
	void *ent;

	/*
 	 * search subscribed event.
 	 */
	key.name = name;
	ent = tfind(&key, &bind_event_root, compare);
	if (ent != NULL)
		return *(struct bind_event_t **)ent; /* found */
	return NULL; /* not found */
}

struct bind_event_t *register_event(const char *name)
{
	struct bind_event_t *evt;
	struct prop_info_t *property_info;
	void *ent;

	property_info = getProperty_dict(name);
	if (property_info == NULL)
	{
		ERRMSG("NOT Supported property:\"%s\".", name);
		return NULL;
	}
	evt = calloc(1, sizeof(*evt));
	if (evt == NULL)
	{
		ERRMSG("not enough memory");
		return NULL;
	}

	evt->event = afb_daemon_make_event(property_info->name);
	if (!afb_event_is_valid(evt->event))
	{
		free(evt);
		ERRMSG("afb_daemon_make_event failed");
		return NULL;
	}

	evt->name = property_info->name;
	evt->raw_info = property_info;
	ent = tsearch(evt, &bind_event_root, compare);
	if (ent == NULL)
	{
		ERRMSG("not enough memory");
		afb_event_drop(evt->event);
		free(evt);
		return NULL;
	}
#ifdef DEBUG /* double check */
	if ((*(struct bind_event_t **)ent) != evt)
		ERRMSG("event \"%s\" is duplicate?", property_info->name);
#endif

	return  evt;
}

int remove_event(const char *name)
{
	struct bind_event_t *dnode;
	struct bind_event_t key;
	void *p;

	key.name = name;
	p = tfind(&key, &bind_event_root, compare);
	if (p == NULL)
	{
		return 0;
	}
	dnode = *(struct bind_event_t **)p;
	afb_event_drop(dnode->event);
	tdelete(&key, &bind_event_root, compare);
	free(dnode);

	return 0;
}
