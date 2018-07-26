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
#include <stdio.h>
#include <stdlib.h>
#include <search.h>
#include <string.h>

#include "af-steering-wheel-binding.h"
#include "prop_search.h"

#define SUPPORT_PROPERTY_LIMIT 512

static void *property_root = NULL;
static const char *support_property_list[SUPPORT_PROPERTY_LIMIT];
static int nProperties = 0;
static void (*_walker_action)(struct prop_info_t *);

static int property_compare(const void *pa, const void *pb)
{
	struct prop_info_t *a =(struct prop_info_t *)pa;
	struct prop_info_t *b =(struct prop_info_t *)pb;
	return strcmp(a->name,b->name);
}

static int addProperty_dict(struct prop_info_t *prop_info)
{
	void *v;
	v = tfind((void *)prop_info, &property_root, property_compare);
	if (v == NULL)
	{
		v = tsearch((void *)prop_info, &property_root, property_compare);
		if (v == NULL)
		{
			ERRMSG("add property failed: not enogh memory?");
			return -1;
		}
		if (nProperties >= SUPPORT_PROPERTY_LIMIT)
		{
			ERRMSG("Reach properties limit");
			return -1;
		}
		support_property_list[nProperties] = prop_info->name;
		++nProperties;
		return 0;
	} /* else  Multi entry */

	return 1;
}

int addAllPropDict(struct wheel_info_t *wheel_info)
{
	unsigned int maxEntry = wheel_info->nData;

	for(unsigned int i=0; i < maxEntry ; i++)
	{
		int ret;
		ret = addProperty_dict(&wheel_info->property[i]);
		if (!ret)
		{
			continue;
		}
	}

	return 0;
}

const char **getSupportPropertiesList(int *nEntry)
{
	*nEntry = nProperties;
	return support_property_list;
}

struct prop_info_t * getProperty_dict(const char *propertyName)
{
	struct prop_info_t key;
	void *v;

	key.name = propertyName;
	v = tfind((void *)&key, &property_root, property_compare);
	if (v == NULL) {
		return NULL;
	}
	return (*(struct prop_info_t **)v);
}

static void canid_walk_action(const void *nodep, const VISIT which , const int depth)
{
	struct prop_info_t *datap;

	switch (which) {
	case preorder:
		break;
	case postorder:
		datap = *(struct prop_info_t **) nodep;
		_walker_action(datap);
		break;
	case endorder:
		break;
	case leaf:
		datap = *(struct prop_info_t **) nodep;
		_walker_action(datap);
		break;
	}
}

void canid_walker(void (*walker_action)(struct prop_info_t *))
{
	if (walker_action == NULL)
		return;
	_walker_action = walker_action;
	twalk(property_root, canid_walk_action);
}
