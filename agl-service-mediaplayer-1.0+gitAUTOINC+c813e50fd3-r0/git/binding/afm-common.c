/*
 * Copyright (C) 2017 Konsulko Group
 * Author: Matt Ranostay <matt.ranostay@konsulko.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define _GNU_SOURCE

#include "afm-common.h"

const char *control_commands[] = {
	"play",
	"pause",
	"previous",
	"next",
	"seek",
	"fast-forward",
	"rewind",
	"pick-track",
	"volume",
	"loop",
	"stop",
};

int get_command_index(const char *name)
{
	int i;

	if (name == NULL)
		return -EINVAL;

	for (i = 0; i < NUM_CMDS; i++) {
		if (!strcasecmp(control_commands[i], name))
			return i;
	}

	return -EINVAL;
}

GList *find_media_index(GList *list, long int index)
{
	struct playlist_item *item;
	GList *l;

	for (l = list; l; l = l->next) {
		item = l->data;

		if (!item)
			continue;

		if (item->id == index)
			return l;
	}

	return NULL;
}

void g_free_playlist_item(void *ptr)
{
	struct playlist_item *item = ptr;

	if (ptr == NULL)
		return;

	g_free(item->title);
	g_free(item->album);
	g_free(item->artist);
	g_free(item->genre);
	g_free(item->media_path);
	g_free(item);
}
