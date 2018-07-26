/*
 *  Copyright 2017 Konsulko Group
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

#ifndef MEDIAPLAYER_MANAGER_H
#define MEDIAPLAYER_MANAGER_H

#include <glib.h>
#include <gio/gio.h>
#include <glib-object.h>

#include "gdbus/lightmediascanner_interface.h"

    /* Debug Trace Level */
#define DT_LEVEL_ERROR          (1 << 1)
#define DT_LEVEL_WARNING        (1 << 2)
#define DT_LEVEL_NOTICE         (1 << 3)
#define DT_LEVEL_INFO           (1 << 4)
#define DT_LEVEL_DEBUG          (1 << 5)
//#define _DEBUG

#define LOGE(fmt, args...)   \
    DebugTraceSendMsg(DT_LEVEL_ERROR, g_strdup_printf("[%d:%s]" fmt, __LINE__, __FUNCTION__, ## args))
#define LOGW(fmt, args...)   \
    DebugTraceSendMsg(DT_LEVEL_WARNING, g_strdup_printf("[%d:%s]" fmt, __LINE__, __FUNCTION__, ## args))
#define LOGN(fmt, args...)   \
    DebugTraceSendMsg(DT_LEVEL_NOTICE,  g_strdup_printf("[%d:%s]" fmt, __LINE__, __FUNCTION__, ## args))
#define LOGI(fmt, args...)   \
    DebugTraceSendMsg(DT_LEVEL_INFO, g_strdup_printf("[%d:%s]" fmt, __LINE__, __FUNCTION__, ## args))
#define LOGD(fmt, args...)   \
    DebugTraceSendMsg(DT_LEVEL_DEBUG,  g_strdup_printf("[%d:%s]" fmt, __LINE__, __FUNCTION__, ## args))

#ifdef _DEBUG
 #define _DEBUG_PRINT_DBUS
 #define LOCAL_PRINT_DEBUG
#endif

#ifdef LOCAL_PRINT_DEBUG
#define D_PRINTF(fmt, args...) \
	g_print("[DEBUG][%d:%s]"fmt,  __LINE__, __FUNCTION__, ## args)
#define D_PRINTF_RAW(fmt, args...) \
	g_print(""fmt, ## args)
#else
#define D_PRINTF(fmt, args...)
#define D_PRINTF_RAW(fmt, args...)
#endif	/* ifdef _DEBUG */

void DebugTraceSendMsg(int level, gchar* message);

//service
#define AGENT_SERVICE               "org.agent"

//remote service
#define LIGHTMEDIASCANNER_SERVICE   "org.lightmediascanner"

//object path
#define LIGHTMEDIASCANNER_PATH      "/org/lightmediascanner/Scanner1"

//interface
#define LIGHTMEDIASCANNER_INTERFACE "org.lightmediascanner.Scanner1"
#define UDISKS_INTERFACE            "org.freedesktop.UDisks"
#define FREEDESKTOP_PROPERTIES      "org.freedesktop.DBus.Properties"

//sqlite
#define AUDIO_SQL_QUERY \
                  "SELECT files.path, audios.title, audio_artists.name, " \
                  "audio_albums.name, audio_genres.name, audios.length " \
                  "FROM files INNER JOIN audios " \
                  "ON files.id = audios.id " \
                  "LEFT JOIN audio_artists " \
                  "ON audio_artists.id = audios.artist_id " \
                  "LEFT JOIN audio_albums " \
                  "ON audio_albums.id = audios.album_id " \
                  "LEFT JOIN audio_genres " \
                  "ON audio_genres.id = audios.genre_id " \
                  "WHERE files.path LIKE '%s/%%' " \
                  "ORDER BY " \
                  "audios.artist_id, audios.album_id, audios.trackno"

#define VIDEO_SQL_QUERY \
                  "SELECT files.path, videos.title, videos.artist, \"\", \"\", " \
                  "videos.length FROM files " \
                  "INNER JOIN videos ON videos.id = files.id " \
                  "WHERE files.path LIKE '%s/%%' " \
                  "ORDER BY " \
                  "videos.title"

typedef struct {
    GList *list;
    gchar *uri_filter;
    GMutex m;
    Scanner1 *lms_proxy;
} stMediaPlayerManage;

typedef struct tagBinding_RegisterCallback
{
    void (*binding_device_added)(GList *list);
    void (*binding_device_removed)(const char *obj_path);
} Binding_RegisterCallback_t;

/* ------ PUBLIC PLUGIN FUNCTIONS --------- */
void BindingAPIRegister(const Binding_RegisterCallback_t* pstRegisterCallback);
int MediaPlayerManagerInit(void);

void ListLock();
void ListUnlock();

GList* media_lightmediascanner_scan(GList *list, gchar *uri, int scan_type);

struct Media_Item {
    gchar *path;
    gint type;
    struct {
        gchar *title;
        gchar *artist;
        gchar *album;
        gchar *genre;
        gint  duration;
    } metadata;
};

enum {
    LMS_AUDIO_SCAN,
    LMS_VIDEO_SCAN,
    LMS_SCAN_COUNT,
};

extern const char *lms_scan_types[LMS_SCAN_COUNT];

#endif
