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


#ifndef _AFM_COMMON_H
#define _AFM_COMMON_H

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <glib.h>
#include <json-c/json.h>

struct playlist_item {
    int id;
    gchar *title;
    gchar *album;
    gchar *artist;
    gchar *genre;
    gint64 duration;
    gchar *media_path;
    gchar *media_type;
};

enum {
    PLAY_CMD = 0,
    PAUSE_CMD,
    PREVIOUS_CMD,
    NEXT_CMD,
    SEEK_CMD,
    FASTFORWARD_CMD,
    REWIND_CMD,
    PICKTRACK_CMD,
    VOLUME_CMD,
    LOOP_CMD,
    STOP_CMD,
    NUM_CMDS
};

const char *control_commands[NUM_CMDS];
int get_command_index(const char *name);
GList *find_media_index(GList *list, long int index);
void g_free_playlist_item(void *ptr);

#endif /* _AFM_COMMON_H */
