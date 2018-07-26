/*
 *  Copyright 2017 Konsulko Group
 *
 *  Based on bluetooth-manager.c
 *   Copyright 2016 ALPS ELECTRIC CO., LTD.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <pthread.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <gio/gio.h>
#include <glib-object.h>
#include <sqlite3.h>

#include "media-manager.h"

const char *lms_scan_types[] = {
    "audio",
    "video",
};

static Binding_RegisterCallback_t g_RegisterCallback = { 0 };
static stMediaPlayerManage MediaPlayerManage = { 0 };

/* ------ LOCAL  FUNCTIONS --------- */
void ListLock() {
    g_mutex_lock(&(MediaPlayerManage.m));
}

void ListUnlock() {
    g_mutex_unlock(&(MediaPlayerManage.m));
}

void DebugTraceSendMsg(int level, gchar* message)
{
#ifdef LOCAL_PRINT_DEBUG
    switch (level)
    {
            case DT_LEVEL_ERROR:
                g_print("[E]");
                break;

            case DT_LEVEL_WARNING:
                g_print("[W]");
                break;

            case DT_LEVEL_NOTICE:
                g_print("[N]");
                break;

            case DT_LEVEL_INFO:
                g_print("[I]");
                break;

            case DT_LEVEL_DEBUG:
                g_print("[D]");
                break;

            default:
                g_print("[-]");
                break;
    }

    g_print("%s",message);
#endif

    if (message) {
        g_free(message);
    }

}

GList* media_lightmediascanner_scan(GList *list, gchar *uri, int scan_type)
{
    sqlite3 *conn;
    sqlite3_stmt *res;
    const char *tail;
    const gchar *db_path;
    gchar *query;
    int ret = 0;

    db_path = scanner1_get_data_base_path(MediaPlayerManage.lms_proxy);

    ret = sqlite3_open(db_path, &conn);
    if (ret) {
        LOGE("Cannot open SQLITE database: '%s'\n", db_path);
        return NULL;
    }

    switch (scan_type) {
    case LMS_VIDEO_SCAN:
        query = g_strdup_printf(VIDEO_SQL_QUERY, uri ? uri : "");
        break;
    case LMS_AUDIO_SCAN:
    default:
        query = g_strdup_printf(AUDIO_SQL_QUERY, uri ? uri : "");
    }

    if (!query) {
        LOGE("Cannot allocate memory for query\n");
        return NULL;
    }

    ret = sqlite3_prepare_v2(conn, query, strlen(query), &res, &tail);
    if (ret) {
        LOGE("Cannot execute query '%s'\n", query);
        g_free(query);
        return NULL;
    }

    while (sqlite3_step(res) == SQLITE_ROW) {
        struct stat buf;
        struct Media_Item *item;
        const char *path = (const char *) sqlite3_column_text(res, 0);

        ret = stat(path, &buf);
        if (ret)
            continue;

        item = g_malloc0(sizeof(*item));
        item->path = g_strdup_printf("file://%s", path);
        item->type = scan_type;
        item->metadata.title = g_strdup((gchar *) sqlite3_column_text(res, 1));
        item->metadata.artist = g_strdup((gchar *) sqlite3_column_text(res, 2));
        item->metadata.album = g_strdup((gchar *) sqlite3_column_text(res, 3));
        item->metadata.genre = g_strdup((gchar *) sqlite3_column_text(res, 4));
        item->metadata.duration = sqlite3_column_int(res, 5) * 1000;
        list = g_list_append(list, item);
    }

    g_free(query);

    return list;
}

static void free_media_item(void *data)
{
    struct Media_Item *item = data;

	g_free(item->metadata.title);
	g_free(item->metadata.artist);
	g_free(item->metadata.album);
	g_free(item->metadata.genre);
	g_free(item->path);
	g_free(item);
}

static void
on_interface_proxy_properties_changed (GDBusProxy *proxy,
                                    GVariant *changed_properties,
                                    const gchar* const  *invalidated_properties)
{
    GVariantIter iter;
    const gchar *key;
    GVariant *subValue;
    const gchar *pInterface;
    GList *list = NULL;

    pInterface = g_dbus_proxy_get_interface_name (proxy);

    if (0 != g_strcmp0(pInterface, LIGHTMEDIASCANNER_INTERFACE))
        return;

    g_variant_iter_init (&iter, changed_properties);
    while (g_variant_iter_next (&iter, "{&sv}", &key, &subValue))
    {
        gboolean val;
        if (0 == g_strcmp0(key,"IsScanning")) {
            g_variant_get(subValue, "b", &val);
            if (val == TRUE)
                return;
        } else if (0 == g_strcmp0(key, "WriteLocked")) {
            g_variant_get(subValue, "b", &val);
            if (val == TRUE)
                return;
        }
    }

    ListLock();

    list = media_lightmediascanner_scan(list, MediaPlayerManage.uri_filter, LMS_AUDIO_SCAN);
    list = media_lightmediascanner_scan(list, MediaPlayerManage.uri_filter, LMS_VIDEO_SCAN);

    g_free(MediaPlayerManage.uri_filter);
    MediaPlayerManage.uri_filter = NULL;

    if (list != NULL && g_RegisterCallback.binding_device_added)
        g_RegisterCallback.binding_device_added(list);

    g_list_free_full(list, free_media_item);

    ListUnlock();
}

static int MediaPlayerDBusInit(void)
{
    GError *error = NULL;

    MediaPlayerManage.lms_proxy = scanner1_proxy_new_for_bus_sync(
        G_BUS_TYPE_SESSION, G_DBUS_PROXY_FLAGS_NONE, LIGHTMEDIASCANNER_SERVICE,
        LIGHTMEDIASCANNER_PATH, NULL, &error);

    if (MediaPlayerManage.lms_proxy == NULL) {
        LOGE("Create LightMediaScanner Proxy failed\n");
        return -1;
    }

    return 0;
}

static void *media_event_loop_thread(void *unused)
{
    GMainLoop *loop = g_main_loop_new(NULL, FALSE);

    if (loop == NULL)
        return NULL;

    g_signal_connect (MediaPlayerManage.lms_proxy,
                      "g-properties-changed",
                      G_CALLBACK (on_interface_proxy_properties_changed),
                      NULL);

    LOGD("g_main_loop_run\n");
    g_main_loop_run(loop);

    return NULL;
}

void
unmount_cb (GFileMonitor      *mon,
            GFile             *file,
            GFile             *other_file,
            GFileMonitorEvent  event,
            gpointer           udata)
{
    gchar *path = g_file_get_path(file);
    gchar *uri = g_strconcat("file://", path, NULL);

    ListLock();

    if (g_RegisterCallback.binding_device_removed &&
        event == G_FILE_MONITOR_EVENT_DELETED) {
        g_RegisterCallback.binding_device_removed(uri);
        g_free(path);
    } else if (event == G_FILE_MONITOR_EVENT_CREATED) {
        MediaPlayerManage.uri_filter = path;
    } else {
        g_free(path);
    }

    g_free(uri);
    ListUnlock();
}

/*
 * Create MediaPlayer Manager Thread
 * Note: mediaplayer-api should do MediaPlayerManagerInit() before any other 
 *       API calls
 * Returns: 0 - success or error conditions
 */
int MediaPlayerManagerInit() {
    pthread_t thread_id;
    GFile *file;
    GFileMonitor *mon;
    int ret;

    g_mutex_init(&(MediaPlayerManage.m));

    file = g_file_new_for_path("/media");
    g_assert(file != NULL);

    mon = g_file_monitor (file, G_FILE_MONITOR_NONE, NULL, NULL);
    g_assert(mon != NULL);
    g_signal_connect (mon, "changed", G_CALLBACK(unmount_cb), NULL);

    ret = MediaPlayerDBusInit();
    if (ret == 0)
        pthread_create(&thread_id, NULL, media_event_loop_thread, NULL);

    return ret;
}

/*
 * Register MediaPlayer Manager Callback functions
 */
void BindingAPIRegister(const Binding_RegisterCallback_t* pstRegisterCallback)
{
    if (NULL != pstRegisterCallback)
    {
        if (NULL != pstRegisterCallback->binding_device_added)
        {
            g_RegisterCallback.binding_device_added =
                pstRegisterCallback->binding_device_added;
        }

        if (NULL != pstRegisterCallback->binding_device_removed)
        {
            g_RegisterCallback.binding_device_removed =
                pstRegisterCallback->binding_device_removed;
        }
    }
}
