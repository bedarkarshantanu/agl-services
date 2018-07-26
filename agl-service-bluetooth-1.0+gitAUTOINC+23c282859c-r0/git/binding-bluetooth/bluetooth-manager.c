/*  Copyright 2016 ALPS ELECTRIC CO., LTD.
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
#include <pthread.h>
#include <fcntl.h>
#include <glib.h>
#include <gio/gio.h>
#include <glib/gstdio.h>
#include <glib-object.h>
#include <syslog.h>
#include <linux/rfkill.h>
#include <sys/socket.h>
#include <sys/time.h>

#include "bluetooth-manager.h"
#include "bluez-client.h"
#include "ofono-client.h"
#include "bluetooth-agent.h"

static Client cli = { 0 };
static Binding_RegisterCallback_t g_RegisterCallback = { 0 };
static stBluetoothManage BluetoothManage = { 0 };
static FILE *logfp= NULL;

/* ------ LOCAL  FUNCTIONS --------- */
void BluetoothManage_InitFlag_Set(gboolean value)
{
    g_mutex_lock(&(BluetoothManage.m));
    BluetoothManage.inited = value;
    g_mutex_unlock(&(BluetoothManage.m));
}

gboolean BluetoothManage_InitFlag_Get(void)
{
    gboolean inited;

    g_mutex_lock(&(BluetoothManage.m));
    inited = BluetoothManage.inited;
    g_mutex_unlock(&(BluetoothManage.m));

    return inited;
}

void devices_list_lock(void)
{
    g_mutex_lock(&(BluetoothManage.m));
}

void devices_list_unlock(void)
{
    g_mutex_unlock(&(BluetoothManage.m));
}

gchar *bt_priority_path()
{
    gchar *path = g_build_filename(g_get_user_config_dir(), "bluetooth",  NULL);
    gchar *file;

    g_mkdir(path, 0664);

    file = g_build_filename(path, "priorities.lst", NULL);
    g_free(path);

    return file;
}

void bt_remove_priority(gchar *bdaddr)
{
    LOGD("\n");
    FILE *fp;
    GSList *item, *tmp;
    gchar *path;

    LOGD("bd_addr: %s\n", bdaddr);

    devices_list_lock();

    item = g_slist_find_custom(BluetoothManage.priorities, bdaddr, g_strcmp0);
    if (item == NULL)
    {
        devices_list_unlock();
        return;
    }
    g_free(item->data);
    tmp = BluetoothManage.priorities = g_slist_remove_link(BluetoothManage.priorities, item);

    path = bt_priority_path();
    fp = g_fopen(path, "w");
    g_free(path);

    if (fp == NULL)
    {
        devices_list_unlock();
        return;
    }

    while (tmp && tmp->data)
    {
        fputs(tmp->data, fp);
        fputs("\n", fp);
        tmp = tmp->next;
    }

    fclose(fp);

    devices_list_unlock();
}

void bt_save_priority(gchar *bdaddr)
{
    LOGD("\n");
    FILE *fp;
    gchar *path;

    LOGD("bd_addr: %s\n", bdaddr);

    devices_list_lock();

    if (g_slist_find(BluetoothManage.priorities, bdaddr))
    {
        devices_list_unlock();
        return;
    }

    path = bt_priority_path();
    fp = g_fopen(path, "a+");
    g_free(path);

    if (fp == NULL)
    {
        devices_list_unlock();
        return;
    }

    fputs(bdaddr, fp);
    fputs("\n", fp);
    fclose(fp);

    BluetoothManage.priorities = g_slist_append(BluetoothManage.priorities, g_strdup(bdaddr));

    devices_list_unlock();
}

GSList *bt_priority_list()
{
    LOGD("\n");
    FILE *fp;
    GSList *list = NULL;
    gchar *path;
    gchar bdaddr[18];

    devices_list_lock();

    path = bt_priority_path();

    fp = g_fopen(path, "r");
    g_free(path);

    if (fp == NULL)
    {
        devices_list_unlock();
        return NULL;
    }

    while (fscanf(fp, "%17s\n", &bdaddr) == 1)
    {
        LOGD("Found priority device %s\n", bdaddr);
        list = g_slist_append(list, g_strdup(bdaddr));
    }

    devices_list_unlock();

    return list;
}

int device_priority_list(void *(object_cb)(void *, gchar *), void *ptr)
{
    GSList *tmp;

    devices_list_lock();

    tmp = BluetoothManage.priorities;

    while (tmp)
    {
        object_cb(ptr, tmp->data);
        tmp = tmp->next;
    }

    devices_list_unlock();

    return 0;
}

static int device_path_cmp(struct btd_device * device, const gchar* pPath )
{
    return !g_str_has_prefix (pPath, device->path);
}

/*
 * search device by path
 * Returns the first found btd_device or NULL if it is not found
 */
struct btd_device *devices_list_find_device_by_path(const gchar* pPath)
{
    GSList * temp;

    temp = g_slist_find_custom (BluetoothManage.device, pPath,
                                (GCompareFunc)device_path_cmp);

    if (temp) {
        return temp->data;
    }

    return NULL;

}


static int device_bdaddr_cmp(struct btd_device * device, const gchar* pBDaddr )
{
    return g_strcmp0 (device->bdaddr, pBDaddr);
}

/*
 * search device by path
 * Returns the first found btd_device or NULL if it is not found
 */
struct btd_device *devices_list_find_device_by_bdaddr(const gchar* pBDaddr)
{
    GSList * temp;

    temp = g_slist_find_custom (BluetoothManage.device, pBDaddr,
                                (GCompareFunc)device_bdaddr_cmp);

    if (temp) {
        return temp->data;
    }

    return NULL;

}


/*
 * make a copy of each element
 * And, to entirely free the new btd_device, you could do: device_free
 */
struct btd_device *device_copy(struct btd_device* device)
{
    struct btd_device * temp;

    if (NULL == device) {
        return NULL;
    }

    temp = g_malloc0(sizeof(struct btd_device));
    temp->path = g_strdup(device->path);
    temp->bdaddr = g_strdup(device->bdaddr);
    temp->name = g_strdup(device->name);
    temp->paired = device->paired;
    temp->trusted = device->trusted;
    temp->connected = device->connected;
    temp->avconnected = device->avconnected;
    temp->hfpconnected = device->hfpconnected;
    temp->uuids = g_list_copy_deep(device->uuids, g_strdup, NULL);

    return temp;
}

/*
 * Frees all of the memory
 */
void device_free(struct btd_device* device)
{

    if (NULL == device) {
        return ;
    }
    D_PRINTF("device %p\n",device);
    if (device->path) {
        D_PRINTF("path:%s\n",device->path);
        g_free(device->path);
        device->path = NULL;
    }
    if (device->bdaddr) {
        D_PRINTF("bdaddr:%s\n",device->bdaddr);
        g_free(device->bdaddr);
        device->bdaddr = NULL;
    }
    if (device->name) {
        D_PRINTF("name:%s\n",device->name);
        g_free(device->name);
        device->name = NULL;
    }

    if (device->avrcp_title) {
        D_PRINTF("avrcp_title:%s\n",device->avrcp_title);
        g_free(device->avrcp_title);
        device->avrcp_title = NULL;
    }

    if (device->avrcp_artist) {
        D_PRINTF("avrcp_artist:%s\n",device->avrcp_artist);
        g_free(device->avrcp_artist);
        device->avrcp_artist = NULL;
    }

    if (device->avrcp_status) {
        D_PRINTF("avrcp_status:%s\n",device->avrcp_status);
        g_free(device->avrcp_status);
        device->avrcp_status = NULL;
    }

    if (device->transport_state) {
        D_PRINTF("transport_state:%s\n",device->transport_state);
        g_free(device->transport_state);
        device->transport_state = NULL;
    }

    if (device->uuids) {
        D_PRINTF("uuids: xxx\n");
        g_list_free_full(device->uuids, g_free);
        device->uuids = NULL;
    }

    g_free(device);
}

void device_print(struct btd_device *BDdevice)
{
    g_print("device %p\n",BDdevice);
    g_print("bdaddr\t\t:%s\n",BDdevice->bdaddr);
    g_print("name\t\t:%s\n",BDdevice->name);
    g_print("trusted\t\t:%d\n",BDdevice->trusted);
    g_print("paired\t\t:%d\n",BDdevice->paired);

    g_print("connected\t:%d\n",BDdevice->connected);
    g_print("AVPconnected\t:%d\n",BDdevice->avconnected);
    g_print("HFPconnected\t:%d\n",BDdevice->hfpconnected);
}

/*
 * remove all the devices
 */
void devices_list_cleanup()
{
    LOGD("\n");
    GSList * temp = BluetoothManage.device;
    while (temp) {
        struct btd_device *BDdevice = temp->data;
        temp = temp->next;

        BluetoothManage.device = g_slist_remove_all(BluetoothManage.device,
                BDdevice);

        device_free(BDdevice);
    }
}

/*
 * Print all the devices
 */
void devices_list_print()
{
    LOGD("\n");
    GSList * temp = BluetoothManage.device;
    while (temp) {
        struct btd_device *BDdevice = temp->data;
        temp = temp->next;
        g_print("----------------------------------------\n");
        device_print(BDdevice);
    }
    g_print("----------------------------------------\n");
}

/*
 * update device from Interfcace org.bluez.MediaControl1 properties
 */
static int device_update_from_MediaControl1(struct btd_device *device,
                                                     GVariant *value)
{
    GVariantIter iter;
    const gchar *key;
    GVariant *subValue;

    if ((NULL==device) || (NULL==value))
    {
        return -1;
    }

    g_variant_iter_init (&iter, value);
    while (g_variant_iter_next (&iter, "{&sv}", &key, &subValue))
    {
        //gchar *s = g_variant_print (subValue, TRUE);
        //g_print ("  %s -> %s\n", key, s);
        //g_free (s);

        gboolean value_b  =  FALSE;//b gboolean
        //gchar value_c = 0;
        //guchar value_y  =  0;//y guchar
        //gint16 value_n  =  0;//n gint16
        //guint16 value_q  =  0;//q guint16
        //gint32 value_i  =  0;//i gint32
        //guint32 value_u  =  0;//u guint32
        //gint64 value_x  =  0;//x gint64
        //guint64 value_t  =  0;//t guint64
        //gint32 value_h  = 0;//h gint32
        //gdouble value_d = 0.0;//d gdouble
        gchar *str;//d gdouble

        if (0==g_strcmp0(key,"Connected")) {
            g_variant_get(subValue, "b", &value_b );
            D_PRINTF("Connected %d\n",value_b);
            device->avconnected = value_b;
        }else if (0==g_strcmp0(key,"Player")) {
            g_variant_get(subValue, "o", &str );
            D_PRINTF("Player Object %s\n",str);
        }
    }

    return 0;
}

/*
 * update device from Interface org.bluez.MediaTransport1 properties
 */
static int device_update_from_MediaTransport1(struct btd_device *device,
                                                     GVariant *value)
{
    GVariantIter iter;
    const gchar *key;
    GVariant *subValue;

    if ((NULL==device) || (NULL==value))
    {
        return -1;
    }

    g_variant_iter_init (&iter, value);
    while (g_variant_iter_next (&iter, "{&sv}", &key, &subValue))
    {
        //gchar *s = g_variant_print (subValue, TRUE);
        //g_print ("  %s -> %s\n", key, s);
        //g_free (s);

        //gboolean value_b  =  FALSE;//b gboolean
        //gchar value_c = 0;
        //guchar value_y  =  0;//y guchar
        //gint16 value_n  =  0;//n gint16
        guint16 value_q  =  0;//q guint16
        //gint32 value_i  =  0;//i gint32
        //guint32 value_u  =  0;//u guint32
        //gint64 value_x  =  0;//x gint64
        //guint64 value_t  =  0;//t guint64
        //gint32 value_h  = 0;//h gint32
        //gdouble value_d = 0.0;//d gdouble
        gchar *str;//d gdouble

        if (0==g_strcmp0(key,"State")) {
            g_variant_get(subValue, "s", &str);
            D_PRINTF("State %s\n", str);
            if (device->transport_state)
                free(device->transport_state);
            device->transport_state = g_strdup(str);
        } else if (0==g_strcmp0(key,"Volume")) {
            g_variant_get(subValue, "q", &value_q);
            D_PRINTF("Volume %d\n", value_q);
            device->transport_volume = value_q;
        }
    }

    return 0;
}


static int device_update_from_Track(struct btd_device *device,
                                                     GVariant *value)
{
    GVariantIter iter;
    const gchar *key;
    GVariant *subValue;

    if ((NULL==device) || (NULL==value))
    {
        return -1;
    }

    g_variant_iter_init (&iter, value);
    while (g_variant_iter_next (&iter, "{&sv}", &key, &subValue))
    {
        //gchar *s = g_variant_print (subValue, TRUE);
        //g_print ("  %s -> %s\n", key, s);
        //g_free (s);

        gboolean value_b  =  FALSE;//b gboolean
        //gchar value_c = 0;
        //guchar value_y  =  0;//y guchar
        //gint16 value_n  =  0;//n gint16
        //guint16 value_q  =  0;//q guint16
        //gint32 value_i  =  0;//i gint32
        guint32 value_u  =  0;//u guint32
        //gint64 value_x  =  0;//x gint64
        //guint64 value_t  =  0;//t guint64
        //gint32 value_h  = 0;//h gint32
        //gdouble value_d = 0.0;//d gdouble
        gchar *str;//d gdouble

        if (0==g_strcmp0(key,"Title")) {
            g_variant_get(subValue, "s", &str);
            D_PRINTF("Title %s\n", str);
            if (device->avrcp_title)
                free(device->avrcp_title);
            device->avrcp_title = g_strdup(str);
        } else if (0==g_strcmp0(key,"Artist")) {
            g_variant_get(subValue, "s", &str);
            D_PRINTF("Artist %s\n", str);
            if (device->avrcp_artist)
                free(device->avrcp_artist);
            device->avrcp_artist = g_strdup(str);
        } else if (0==g_strcmp0(key,"Duration")) {
            g_variant_get(subValue, "u", &value_u);
            D_PRINTF("Duration %u\n", value_u);
            device->avrcp_duration = value_u;
        }
    }

    return 0;
}

/*
 * update device from Interface org.bluez.MediaPlayer1 properties
 */
static int device_update_from_MediaPlayer1(struct btd_device *device,
                                                     GVariant *value)
{
    GVariantIter iter;
    const gchar *key;
    GVariant *subValue;

    if ((NULL==device) || (NULL==value))
    {
        return -1;
    }

    g_variant_iter_init (&iter, value);
    while (g_variant_iter_next (&iter, "{&sv}", &key, &subValue))
    {
        //gchar *s = g_variant_print (subValue, TRUE);
        //g_print ("  %s -> %s\n", key, s);
        //g_free (s);

        gboolean value_b  =  FALSE;//b gboolean
        //gchar value_c = 0;
        //guchar value_y  =  0;//y guchar
        //gint16 value_n  =  0;//n gint16
        //guint16 value_q  =  0;//q guint16
        //gint32 value_i  =  0;//i gint32
        guint32 value_u  =  0;//u guint32
        //gint64 value_x  =  0;//x gint64
        //guint64 value_t  =  0;//t guint64
        //gint32 value_h  = 0;//h gint32
        //gdouble value_d = 0.0;//d gdouble
        gchar *str;//d gdouble

        if (0==g_strcmp0(key,"Status")) {
            g_variant_get(subValue, "s", &str);
            D_PRINTF("Status %s\n", str);
            if (device->avrcp_status)
                free(device->avrcp_status);
            device->avrcp_status = g_strdup(str);
        } else if (0==g_strcmp0(key,"Position")) {
            g_variant_get(subValue, "u", &value_u);
            D_PRINTF("Position %d\n", value_u);
            device->avrcp_position = value_u;
        } else if (0==g_strcmp0(key,"Track")) {
            device_update_from_Track(device, subValue);
        }
    }

    return 0;
}

/*
 * update device from Interfcace org.bluez.Device1 properties
 */
static int device_update_from_Device1(struct btd_device *device,
                                               GVariant *value)
{
    if ((NULL==device) || (NULL==value))
    {
        return -1;
    }

    GVariantIter iter;
    const gchar *key;
    GVariant *subValue;

    g_variant_iter_init (&iter, value);
    while (g_variant_iter_next (&iter, "{&sv}", &key, &subValue))
    {
        //gchar *s = g_variant_print (subValue, TRUE);
        //g_print ("  %s -> %s\n", key, s);
        //g_free (s);

        gboolean value_b  =  FALSE;//b gboolean
        //gchar value_c = 0;
        //guchar value_y  =  0;//y guchar
        //gint16 value_n  =  0;//n gint16
        //guint16 value_q  =  0;//q guint16
        //gint32 value_i  =  0;//i gint32
        //guint32 value_u  =  0;//u guint32
        //gint64 value_x  =  0;//x gint64
        //guint64 value_t  =  0;//t guint64
        //gint32 value_h  = 0;//h gint32
        //gdouble value_d = 0.0;//d gdouble
        gchar *str;//d gdouble

        if (0==g_strcmp0(key,"Address")) {
            g_variant_get(subValue, "s", &str );
            D_PRINTF("Address %s\n",str);

            if (device->bdaddr)
                g_free (device->bdaddr);

            device->bdaddr = g_strdup(str);

            g_free (str);
            str = NULL;

        }else if (0==g_strcmp0(key,"Name")) {
            g_variant_get(subValue, "s", &str );
            D_PRINTF("Name %s\n",str);

            if (device->name)
                g_free (device->name);

            device->name = g_strdup(str);

            g_free (str);
            str = NULL;

        }else if (0==g_strcmp0(key,"Paired")) {
            g_variant_get(subValue, "b", &value_b );
            D_PRINTF("Paired %d\n",value_b);
            device->paired = value_b;
        }else if (0==g_strcmp0(key,"Trusted")) {
            g_variant_get(subValue, "b", &value_b );
            D_PRINTF("Trusted %d\n",value_b);
            device->trusted = value_b;
         }else if (0==g_strcmp0(key,"Connected")) {
            g_variant_get(subValue, "b", &value_b );
            D_PRINTF("Connected %d\n",value_b);
            device->connected = value_b;
        }
    }

    return 0;

}
#if 0
/*
 * update device from Interfcace org.bluez.MediaControl1 properties
 */
static int bluez_mediacontrol1_properties_update(struct bt_device *device,
                                                         GVariant *value)
{
    GVariantIter iter;
    const gchar *key;
    GVariant *subValue;

    if ((NULL==device) || (NULL==value))
    {
        return -1;
    }

    g_variant_iter_init (&iter, value);
    while (g_variant_iter_next (&iter, "{&sv}", &key, &subValue))
    {
        //gchar *s = g_variant_print (subValue, TRUE);
        //g_print ("  %s -> %s\n", key, s);
        //g_free (s);

        gboolean value_b  =  FALSE;//b gboolean
        //gchar value_c = 0;
        //guchar value_y  =  0;//y guchar
        gint16 value_n  =  0;//n gint16
        //guint16 value_q  =  0;//q guint16
        //gint32 value_i  =  0;//i gint32
        //guint32 value_u  =  0;//u guint32
        //gint64 value_x  =  0;//x gint64
        //guint64 value_t  =  0;//t guint64
        //gint32 value_h  = 0;//h gint32
        //gdouble value_d = 0.0;//d gdouble
        gchar *str;//d gdouble

        if (0==g_strcmp0(key,"Connected")) {
            g_variant_get(subValue, "b", &value_b );
            D_PRINTF("Connected %d\n",value_b);
            device->avconnected = value_b;

        }else if (0==g_strcmp0(key,"Player")) {
            g_variant_get(subValue, "o", &str );
            D_PRINTF("Player Object %s\n",str);

        }
    }

    return 0;

}
#endif

/*
 * make a copy of each element
 */
struct btd_device *device_copy_from_bluez(struct bt_device* device)
{
    struct btd_device * temp;

    if (NULL == device) {
        return NULL;
    }

    temp = g_malloc0(sizeof(struct btd_device));
    temp->path = g_strdup(device->path);
    temp->bdaddr = g_strdup(device->bdaddr);
    temp->name = g_strdup(device->name);
    temp->paired = device->paired;
    temp->trusted = device->trusted;
    temp->connected = device->connected;
    temp->avconnected = device->avconnected;
    temp->uuids = g_list_copy_deep(device->uuids, g_strdup, NULL);

    return temp;
}

/*
 * Force Update the device list
 * Call <method>GetManagedObjects
 * Returns: 0 - success or other errors
 */
int devices_list_update(void)
{
    LOGD("\n");

    GSList* list;
    GSList* tmp;
    list = GetBluezDevicesList();

    if (NULL == list)
    {
        return -1;
    }

    tmp = list;

    devices_list_lock();

    devices_list_cleanup();

    while(tmp)
    {
        struct bt_device *BDdevice = tmp->data;
        tmp = tmp->next;

        struct btd_device * new_device = device_copy_from_bluez(BDdevice);

        if (new_device)
        {
            gchar * ofono_path = g_strconcat("/hfp", new_device->path, NULL);
            if (ofono_path)
                new_device->hfpconnected =
                                    getOfonoModemPoweredByPath(ofono_path);

            //BluetoothManage.device = g_slist_prepend(BluetoothManage.device, new_device);
            BluetoothManage.device =
                            g_slist_append(BluetoothManage.device, new_device);
        }

    }

    FreeBluezDevicesList(list);

    devices_list_unlock();


    return 0;
}


/*
 * notify::name-owner callback function
 */
static void bluez_device_added_cb(struct bt_device *device)
{
    LOGD("\n");

    struct btd_device * new_device;

    if (NULL == device)
    {
        return;
    }

    new_device = device_copy_from_bluez(device);

    devices_list_lock();

    //BluetoothManage.device = g_slist_prepend(BluetoothManage.device, new_device);
    BluetoothManage.device = g_slist_append(BluetoothManage.device,new_device);
    if (NULL != g_RegisterCallback.binding_device_added)
    {
        g_RegisterCallback.binding_device_added(new_device);
    }

    devices_list_unlock();

}

/*
 * object-removed callback function
 */
static void bluez_device_removed_cb (const gchar *path)
{
    struct btd_device *device;

    LOGD("%s\n",path);

    devices_list_lock();

    device = devices_list_find_device_by_path(path);

    if (device) {
        BluetoothManage.device = g_slist_remove_all(BluetoothManage.device,
                                    device);

        if (NULL != g_RegisterCallback.binding_device_removed)
        {
            g_RegisterCallback.binding_device_removed(device);
        }

        device_free(device);
    }

    devices_list_unlock();

}

/*
 * BLUEZ interface-proxy-properties-changed callback function
 */
static void
bluez_device_properties_changed_cb (const gchar *pObjecPath,
                                    const gchar *pInterface,
                                    GVariant *properties)
{

    struct btd_device *device;

#if  0
    gchar *s;
    g_print ("Path:%s, Interface:%s\n",pObjecPath, pInterface);
    g_print ("type '%s'\n", g_variant_get_type_string (properties));
    s = g_variant_print (properties, TRUE);
    g_print (" %s\n", s);
    g_free (s);
#endif

    LOGD("%s\n",pObjecPath);

    devices_list_lock();

    device = devices_list_find_device_by_path(pObjecPath);

    if (0 == g_strcmp0(pInterface, DEVICE_INTERFACE)) {

        device_update_from_Device1(device, properties);

    } else if (0 == g_strcmp0(pInterface, MEDIA_CONTROL1_INTERFACE)) {

        device_update_from_MediaControl1(device, properties);

    } else if (0 == g_strcmp0(pInterface, MEDIA_PLAYER1_INTERFACE)) {

        device_update_from_MediaPlayer1(device, properties);

    } else if (0 == g_strcmp0(pInterface, MEDIA_TRANSPORT1_INTERFACE)) {

	device_update_from_MediaTransport1(device, properties);

    }

    if (g_RegisterCallback.binding_device_properties_changed)
        g_RegisterCallback.binding_device_properties_changed(device);

    devices_list_unlock();

}

void ofono_modem_added_cb(struct ofono_modem *modem)
{
    struct btd_device * device;
    gchar *path;

    path = modem->path;

    LOGD("%s\n",path);

    if (NULL == path)
        return;

    devices_list_lock();
    device = devices_list_find_device_by_path(path+4);

    if (device)
    {
        gboolean old_value = device->hfpconnected;

        device->hfpconnected = modem->powered;

        if ((NULL != g_RegisterCallback.binding_device_properties_changed)
            && (old_value != device->hfpconnected))
        {
            g_RegisterCallback.binding_device_properties_changed(device);
        }
    }
    devices_list_unlock();

}

void ofono_modem_removed_cb(struct ofono_modem *modem)
{
    struct btd_device * device;
    gchar *path = modem->path;

    LOGD("%s\n",path);

    if (NULL == path)
        return;

    devices_list_lock();
    device = devices_list_find_device_by_path(path+4);

    if (device)
    {
        gboolean old_value = device->hfpconnected;

        device->hfpconnected = FALSE;

        if ((NULL != g_RegisterCallback.binding_device_properties_changed)
            && (old_value != device->hfpconnected))
        {
            g_RegisterCallback.binding_device_properties_changed(device);
        }
    }
    devices_list_unlock();
}

void ofono_modem_properties_change_cb(struct ofono_modem *modem)
{
    struct btd_device * device;
    gchar *path = modem->path;

    LOGD("%s\n",path);

    if (NULL == path)
        return;

    devices_list_lock();
    device = devices_list_find_device_by_path(path+4);

    if (device)
    {
        gboolean old_value = device->hfpconnected;

        device->hfpconnected = modem->powered;

        if ((NULL != g_RegisterCallback.binding_device_properties_changed)
            && (old_value != device->hfpconnected))
        {
            g_RegisterCallback.binding_device_properties_changed(device);
        }
    }
    devices_list_unlock();
}

gboolean agent_requset_confirm( const gchar *device_path,
                                guint passkey,
                                const gchar **error)
{
    gboolean ret = FALSE;

    const gchar *myerror = ERROR_BLUEZ_REJECT;

    LOGD("-%s,%d\n",device_path,passkey);

    if (NULL != g_RegisterCallback.binding_request_confirmation)
    {

        devices_list_lock();
        struct btd_device *device = devices_list_find_device_by_path(device_path);
        gchar *device_bdaddr = NULL;

        if (device)
        {
            device_bdaddr = g_strdup(device->bdaddr);
        }
        devices_list_unlock();

        if (device_bdaddr)
            ret = g_RegisterCallback.binding_request_confirmation(device_bdaddr, passkey);
    }

    if (TRUE == ret)
    {
        LOGD("return TRUE\n");
        return TRUE;
    }else{
        *error = myerror;
        LOGD("return FALSE\n");
        return FALSE;
    }
}


/*
 * register callback function
 * Returns: 0 - success or other errors
 */
static int bt_manager_app_init(void)
{
    GError *error = NULL;
    int ret;

    cli.system_conn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
    if (error) {
        LOGE("Create System GDBusconnection fail\n");
        LOGE("Error:%s\n", error->message);
        g_error_free(error);
        return -1;
    }

    cli.session_conn = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
    if (error) {
        LOGE("Create Session GDBusconnection fail\n");
        LOGE("Error:%s\n", error->message);
        g_error_free(error);
        g_object_unref(cli.system_conn);
        return -1;
    }

    Bluez_RegisterCallback_t Bluez_API_Callback;
    Bluez_API_Callback.device_added = bluez_device_added_cb;
    Bluez_API_Callback.device_removed = bluez_device_removed_cb;
    Bluez_API_Callback.device_properties_changed = bluez_device_properties_changed_cb;
    BluezDeviceAPIRegister(&Bluez_API_Callback);

    Ofono_RegisterCallback_t Ofono_API_Callback;
    Ofono_API_Callback.modem_added = ofono_modem_added_cb;
    Ofono_API_Callback.modem_removed = ofono_modem_removed_cb;
    Ofono_API_Callback.modem_properties_changed = ofono_modem_properties_change_cb;
    OfonoModemAPIRegister(&Ofono_API_Callback);


    Agent_RegisterCallback_t AgentRegCallback;
    AgentRegCallback.agent_RequestConfirmation = agent_requset_confirm;
    agent_API_register(&AgentRegCallback);

    BluetoothManage.priorities = bt_priority_list();

    ret = BluezManagerInit();
    if (0 != ret )
    {
        LOGE("BluezManagerInit fail\n");
        return -1;
    }

    ret = OfonoManagerInit();
    if (0 != ret )
    {
        LOGE("OfonoManagerInit fail\n");

        BluezManagerQuit();
        return -1;
    }

    ret = agent_register("");
    if (0 != ret )
    {
        LOGE("agent_register fail\n");

        BluezManagerQuit();
        OfonoManagerQuit();
        return -1;
    }

    return 0;
}

int hci_interface_enable(int hdev)
{
    int ctl = socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI);
    int ret;

    if (ctl < 0)
        return ctl;

    ret = ioctl(ctl, HCIDEVUP, hdev);

    close(ctl);

    return ret;
}

gboolean bluetooth_rfkill_event(GIOChannel *chan, GIOCondition cond, gpointer data)
{
    LOGD("\n");
    struct rfkill_event event;
    gchar *name, buf[8];
    int fd, len;

    fd = g_io_channel_unix_get_fd(chan);
    len = read(fd, &event, sizeof(struct rfkill_event));

    if (len != sizeof(struct rfkill_event))
        return TRUE;

    if (event.type != RFKILL_TYPE_BLUETOOTH)
        return TRUE;

    if (event.op == RFKILL_OP_DEL)
        return TRUE;

    name = g_strdup_printf("/sys/class/rfkill/rfkill%u/soft", event.idx);

    fd = g_open(name, O_WRONLY);
    write(fd, "0", 1);
    g_close(fd, NULL);

    g_free(name);

    memset(&buf, 0, sizeof(buf));

    name = g_strdup_printf("/sys/class/rfkill/rfkill%u/name", event.idx);
    fd = g_open(name, O_RDONLY);
    len = read(fd, &buf, sizeof(buf) - 1);

    if (len > 0)
    {
        int idx = 0;
        sscanf(buf, "hci%d", &idx);

        /*
         * 50 millisecond delay to allow time for rfkill to unblock before
         * attempting to bring up HCI interface
         */
        g_usleep(50000);

        LOGD ("Enabling hci%d interface\n", idx);
        hci_interface_enable(idx);
    }

    g_free(name);

    return TRUE;
}


/* Create RFKILL monitor to soft unblock bluetooth devices
 *
 * Returns: 0 - success or other errors
 */
int BluetoothMonitorInit()
{
    int fd = g_open("/dev/rfkill", O_RDWR);
    GIOChannel *chan = NULL;

    if (fd < 0)
    {
        LOGE("Cannot open /dev/rfkill");
        return -1;
    }

    chan = g_io_channel_unix_new(fd);
    g_io_channel_set_close_on_unref(chan, TRUE);

    BluetoothManage.watch = g_io_add_watch(chan, G_IO_IN, bluetooth_rfkill_event, NULL);

    g_io_channel_unref(chan);

    return 0;
}

gboolean bt_autoconnect(gpointer ptr)
{
    LOGD("\n");
    gboolean ret = TRUE;
    GSList *list, *tmp;

    devices_list_lock();

    list = GetBluezDevicesList();
    tmp = list;

    while (tmp)
    {
        struct bt_device *BDdevice = tmp->data;
        tmp = tmp->next;

        if (BDdevice->connected)
        {
            g_slist_free_full(list, g_free);
            devices_list_unlock();
            return FALSE;
        }
    }

    g_slist_free_full(list, g_free);
    devices_list_unlock();

    tmp = BluetoothManage.priorities;

    if (tmp == NULL)
    {
        return TRUE;
    }

    while (tmp)
    {
        LOGD("Autoconnect to %s\n", tmp->data);
        ret = device_connect(tmp->data, NULL) ? TRUE : FALSE;
        tmp = tmp->next;

        if (ret == FALSE)
        {
            setHMIStatus(ACTIVE);
            break;
        }
    }

    return ret;
}

/*
 * Bluetooth Manager Thread
 * register callback function and create a new GMainLoop structure
 */
static void *bt_event_loop_thread()
{
    int ret = 0;

    cli.clientloop = g_main_loop_new(NULL, FALSE);

    ret = bt_manager_app_init();

    if (0 == ret){

        devices_list_update();

        BluetoothManage_InitFlag_Set(TRUE);
        BluetoothMonitorInit();

        BluetoothManage.autoconnect = g_timeout_add_seconds(5, bt_autoconnect, NULL);

        LOGD("g_main_loop_run\n");
        g_main_loop_run(cli.clientloop);

    }

    g_main_loop_unref(cli.clientloop);

    LOGD("Exit\n");
}

/*
 * print log message
 */
void DebugTraceSendMsg(int level, gchar* message)
{

    if (logfp)
    {
        struct timeval  tv;
        struct tm       tm;
        char            s[32]       = {0};

        gettimeofday(&tv, NULL);
        localtime_r(&tv.tv_sec, &tm);
        strftime(s, sizeof(s), "%Y-%m-%d %H:%M:%S", &tm);
        fprintf(logfp, "[%s.%6.6d] ", s, (int)(tv.tv_usec));

        switch (level)
        {
                case DT_LEVEL_ERROR:
                    fprintf(logfp,"[E]");
                    break;

                case DT_LEVEL_WARNING:
                    fprintf(logfp,"[W]");
                    break;

                case DT_LEVEL_NOTICE:
                    fprintf(logfp,"[N]");
                    break;

                case DT_LEVEL_INFO:
                    fprintf(logfp,"[I]");
                    break;

                case DT_LEVEL_DEBUG:
                    fprintf(logfp,"[D]");
                    break;

                default:
                    fprintf(logfp,"[-]");
                    break;
        }

        fprintf(logfp,"%s\n",message);
        fflush(logfp);
    }
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



/* ------ PUBLIC PLUGIN FUNCTIONS --------- */

/*
 * Set the Bluez Adapter Property "Powered" value
 * If success return 0, else return -1;
 */
int adapter_set_powered(gboolean powervalue)
{
    LOGD("value:%d\n",powervalue);

    GError *error = NULL;
    GVariant *value;

    if (FALSE == BluetoothManage_InitFlag_Get()) {
        LOGD("BluetoothManage Not Init\n");
        return -1;
    }

    value = g_dbus_connection_call_sync(cli.system_conn, BLUEZ_SERVICE,
                    ADAPTER_PATH,    FREEDESKTOP_PROPERTIES,
                    "Set",    g_variant_new("(ssv)", ADAPTER_INTERFACE,
                        "Powered",  g_variant_new("b", powervalue)),
                    NULL,  G_DBUS_CALL_FLAGS_NONE, DBUS_REPLY_TIMEOUT,
                    NULL, &error);

    if (NULL == value) {
        LOGW ("Error getting object manager client: %s", error->message);
        g_error_free(error);
        return -1;
    }

    g_variant_unref(value);
    return 0;
}

/*
 * Get the Bluez Adapter Property "Powered" value
 * If success return 0, else return -1;
 */
int adapter_get_powered(gboolean *powervalue) {
    LOGD("\n");

    GError *error = NULL;
    GVariant *value = NULL;
    GVariant *subValue = NULL;

    if (FALSE == BluetoothManage_InitFlag_Get()) {
        LOGD("BluetoothManage Not Init\n");
        return -1;
    }

    if (NULL == powervalue) {
        LOGD("powervalue is NULL\n");
        return -1;
    }

    value = g_dbus_connection_call_sync(cli.system_conn, BLUEZ_SERVICE,
                ADAPTER_PATH,   FREEDESKTOP_PROPERTIES, "Get",
                g_variant_new("(ss)", ADAPTER_INTERFACE, "Powered"),
                NULL, G_DBUS_CALL_FLAGS_NONE, DBUS_REPLY_TIMEOUT,
                NULL, &error);

    if (NULL == value) {
        LOGW ("Error getting object manager client: %s\n", error->message);
        g_error_free(error);
        return -1;
    }

    g_variant_get(value, "(v)", &subValue);
    g_variant_get(subValue, "b", powervalue);
    g_variant_unref(subValue);

    g_variant_unref(value);

    LOGD("get ret :%d\n",*powervalue);
    return 0;
}

/*
 * Set the Bluez Adapter Property value
 * Only support boolean property now.(Discoverable, Pairable, Powered)
 * If success return 0, else return -1;
 */
int adapter_set_property(const gchar* property, gboolean setvalue)
{
    LOGD("property:%s,value:%d\n",property, setvalue);

    GError *error = NULL;
    GVariant *value;

    if (FALSE == BluetoothManage_InitFlag_Get()) {
        LOGW("BluetoothManage Not Init\n");
        return -1;
    }

    if ((0!=g_strcmp0 (property, "Discoverable"))&&
        (0!=g_strcmp0 (property, "Pairable"))&&
        (0!=g_strcmp0 (property, "Powered")))
    {
        LOGD("Invalid value\n");
        return -1;
    }
    value = g_dbus_connection_call_sync(cli.system_conn, BLUEZ_SERVICE,
                    ADAPTER_PATH,    FREEDESKTOP_PROPERTIES,
                    "Set",    g_variant_new("(ssv)", ADAPTER_INTERFACE,
                        property,  g_variant_new("b", setvalue)),
                    NULL,  G_DBUS_CALL_FLAGS_NONE, DBUS_REPLY_TIMEOUT,
                    NULL, &error);

    if (NULL == value) {
        LOGW ("Error : %s", error->message);
        g_error_free(error);
        return -1;
    }

    g_variant_unref(value);

    return 0;
}


/*
 * Call the Bluez Adapter Method "StartDiscovery"
 * If success return 0, else return -1;
 */
int adapter_start_discovery()
{
    LOGD("\n");

    GError *error = NULL;
    GVariant *value = NULL;

    if (FALSE == BluetoothManage_InitFlag_Get()) {
        LOGW("BluetoothManage Not Init\n");
        return -1;
    }

    if (BluetoothManage.autoconnect > 0) {
        LOGW("Canceling autoconnection for discovery mode\n");
        g_source_remove(BluetoothManage.autoconnect);
    }

    value = g_dbus_connection_call_sync(cli.system_conn, BLUEZ_SERVICE,
                ADAPTER_PATH,  ADAPTER_INTERFACE, "StartDiscovery",
                NULL, NULL, G_DBUS_CALL_FLAGS_NONE,
                DBUS_REPLY_TIMEOUT, NULL, &error);

    if (NULL == value) {
        LOGW ("Error getting object manager client: %s\n", error->message);
        g_error_free(error);
        return -1;
    }

    g_variant_unref(value);
    return 0;
}

/*
 * Call the Bluez Adapter Method "StopDiscovery"
 * If success return 0, else return -1;
 */
int adapter_stop_discovery()
{
    LOGD("\n");

    GError *error = NULL;
    GVariant *value = NULL;

    if (FALSE == BluetoothManage_InitFlag_Get()) {
        LOGW("BluetoothManage Not Init\n");
        return -1;
    }

    value = g_dbus_connection_call_sync(cli.system_conn, BLUEZ_SERVICE,
                ADAPTER_PATH, ADAPTER_INTERFACE, "StopDiscovery",
                NULL, NULL, G_DBUS_CALL_FLAGS_NONE,
                DBUS_REPLY_TIMEOUT, NULL, &error);

    if (NULL == value) {
        LOGW ("Error getting object manager client: %s\n", error->message);
        g_error_free(error);
        return -1;
    }

    g_variant_unref(value);

    return 0;
}

/*
 * Call the Bluez Adapter Method "RemoveDevice"
 * If success return 0, else return -1;
 */
int adapter_remove_device(const gchar* bdaddr)
{
    LOGD("\n%s\n",bdaddr);

    struct btd_device * device;
    gchar *path;
    GError *error = NULL;
    GVariant *value;


    if (FALSE == BluetoothManage_InitFlag_Get()) {
        LOGW("BluetoothManage Not Init\n");
        return -1;
    }

    devices_list_lock();
    device = devices_list_find_device_by_bdaddr(bdaddr);

    if (NULL == device) {
        devices_list_unlock();
        LOGD("not find device\n");
        return -1;
    }
    path = g_strdup(device->path);
    devices_list_unlock();

    value = g_dbus_connection_call_sync(cli.system_conn, BLUEZ_SERVICE,
                ADAPTER_PATH, ADAPTER_INTERFACE, "RemoveDevice",
                g_variant_new("(o)", path), NULL,
                G_DBUS_CALL_FLAGS_NONE, DBUS_REPLY_TIMEOUT, NULL, &error);

    bt_remove_priority(bdaddr);

    g_free(path);

    if (NULL == value) {
        LOGW ("Error getting object manager client: %s", error->message);
        g_error_free(error);
        return -1;
    }

    g_variant_unref(value);
    return 0;

}


/*
 * Get the copy of device list.
 */
GSList* adapter_get_devices_list()
{
    GSList* tmp;
    devices_list_lock();
    tmp = g_slist_copy_deep (BluetoothManage.device,
                                        (GCopyFunc)device_copy, NULL);
    devices_list_unlock();
    return tmp;
}

/*
 * free device list.
 */
void adapter_devices_list_free(GSList* list)
{
    if (NULL != list)
        g_slist_free_full(list,(GDestroyNotify)device_free);

}

/*
 * device_pair callback
 */
void device_pair_done_cb(GDBusConnection *source_object,
                        GAsyncResult *res,
                        gpointer user_data)
{
    LOGD("\n");
    gchar *bdaddr = user_data;

    if (g_task_had_error(G_TASK(res)) == FALSE)
    {
        bt_save_priority(bdaddr);
    }
    g_free(bdaddr);

    g_dbus_connection_call_finish (source_object, res, NULL);

}

/*
 * send pairing command
 * If success return 0, else return -1;
 */
int device_pair(const gchar * bdaddr)
{
    LOGD("\n%s\n",bdaddr);

    struct btd_device * device;
    gchar *path;
    GError *error = NULL;
    GVariant *value;


    if (FALSE == BluetoothManage_InitFlag_Get()) {
        LOGD("BluetoothManage Not Init\n");
        return -1;
    }

    devices_list_lock();
    device = devices_list_find_device_by_bdaddr(bdaddr);

    if (NULL == device) {
        devices_list_unlock();
        LOGD("not find device\n");
        return -1;
    }
    path = g_strdup(device->path);
    devices_list_unlock();

#if 0
    value = g_dbus_connection_call_sync(cli.system_conn, BLUEZ_SERVICE,
                path, DEVICE_INTERFACE, "Pair",
                NULL, NULL, G_DBUS_CALL_FLAGS_NONE,
                DBUS_REPLY_TIMEOUT, NULL, &error);

    g_free(path);

    if (NULL == value) {
        LOGW ("Error getting object manager client: %s", error->message);
        g_error_free(error);
        return -1;
    }

    g_variant_unref(value);
#else

    g_dbus_connection_call(cli.system_conn, BLUEZ_SERVICE,
                path, DEVICE_INTERFACE, "Pair",
                NULL, NULL, G_DBUS_CALL_FLAGS_NONE,
                DBUS_REPLY_TIMEOUT, NULL,
                (GAsyncReadyCallback)device_pair_done_cb, g_strdup(bdaddr));

    g_free(path);
#endif
    return 0;

}

/*
 * send cancel pairing command
 * If success return 0, else return -1;
 */
int device_cancelPairing(const gchar * bdaddr)
{
    LOGD("\n%s\n",bdaddr);

    struct btd_device * device;
    gchar *path;
    GError *error = NULL;
    GVariant *value;


    if (FALSE == BluetoothManage_InitFlag_Get()) {
        LOGD("BluetoothManage Not Init\n");
        return -1;
    }

    devices_list_lock();
    device = devices_list_find_device_by_bdaddr(bdaddr);

    if (NULL == device) {
        devices_list_unlock();
        LOGD("not find device\n");
        return -1;
    }
    path = g_strdup(device->path);
    devices_list_unlock();

    value = g_dbus_connection_call_sync(cli.system_conn, BLUEZ_SERVICE,
                path, DEVICE_INTERFACE, "CancelPairing",
                NULL, NULL, G_DBUS_CALL_FLAGS_NONE,
                DBUS_REPLY_TIMEOUT, NULL, &error);

    g_free(path);

    if (NULL == value) {
        LOGW ("Error getting object manager client: %s", error->message);
        g_error_free(error);
        return -1;
    }

    g_variant_unref(value);
    return 0;

}
/*
 * send connect command
 * If success return 0, else return -1;
 */
int device_connect(const gchar * bdaddr, const gchar * uuid)
{
    LOGD("\n%s\n",bdaddr);

    struct btd_device * device;
    gchar *path;
    GError *error = NULL;
    GVariant *value;


    if (FALSE == BluetoothManage_InitFlag_Get()) {
        LOGD("BluetoothManage Not Init\n");
        return -1;
    }

    devices_list_lock();
    device = devices_list_find_device_by_bdaddr(bdaddr);

    if (NULL == device) {
        devices_list_unlock();
        LOGD("not find device\n");
        return -1;
    }
    path = g_strdup(device->path);
    devices_list_unlock();

    value = g_dbus_connection_call_sync(cli.system_conn, BLUEZ_SERVICE,
                path, DEVICE_INTERFACE, uuid ? "ConnectProfile" : "Connect",
                uuid ? g_variant_new("(s)", uuid) : NULL,
                NULL, G_DBUS_CALL_FLAGS_NONE,
                DBUS_REPLY_TIMEOUT, NULL, &error);

    g_free(path);

    if (NULL == value) {
        LOGW ("Error getting object manager client: %s", error->message);
        g_error_free(error);
        return -1;
    }

    g_variant_unref(value);
    return 0;

}

/*
 * send disconnect command
 * If success return 0, else return -1;
 */
int device_disconnect(const gchar* bdaddr, const gchar *uuid)
{
    LOGD("\n%s\n",bdaddr);

    struct btd_device * device;
    gchar *path;
    GError *error = NULL;
    GVariant *value;


    if (FALSE == BluetoothManage_InitFlag_Get()) {
        LOGD("BluetoothManage Not Init\n");
        return -1;
    }

    devices_list_lock();
    device = devices_list_find_device_by_bdaddr(bdaddr);

    if (NULL == device) {
        devices_list_unlock();
        LOGD("not find device\n");
        return -1;
    }
    path = g_strdup(device->path);
    devices_list_unlock();

    value = g_dbus_connection_call_sync(cli.system_conn, BLUEZ_SERVICE,
                path, DEVICE_INTERFACE, uuid ? "DisconnectProfile" : "Disconnect",
                uuid ? g_variant_new("(s)", uuid) : NULL,
                NULL, G_DBUS_CALL_FLAGS_NONE,
                DBUS_REPLY_TIMEOUT, NULL, &error);

    g_free(path);

    if (NULL == value) {
        LOGW ("Error getting object manager client: %s", error->message);
        g_error_free(error);
        return -1;
    }

    g_variant_unref(value);
    return 0;
}

/*
 * set remote device property
 * If success return 0, else return -1;
 */
int device_set_property(const char * bdaddr, const char *property_name,
                        const char *property_value)
{
    LOGD("\n%s-%s-%s\n",bdaddr,property_name,property_value);

    GError *error = NULL;
    GVariant *ret;
    struct btd_device * device;
    gchar *path;
    gboolean value;

    if (FALSE == BluetoothManage_InitFlag_Get()) {
        LOGD("BluetoothManage Not Init\n");
        return -1;
    }

    //Only support set "Trusted"
    if (strcmp(property_name, "Trusted")) {
        LOGD("Not support property name\n");
        return -1;
    }

    if (atoi(property_value) == 1 || !strcasecmp(property_value, "true")) {
        value = TRUE;
    } else if (atoi(property_value) == 0
            || !strcasecmp(property_value, "false")) {
        value = FALSE;
    } else {
        LOGD("Not support property value\n");
        return -1;
    }

    devices_list_lock();
    device = devices_list_find_device_by_bdaddr(bdaddr);

    if (NULL == device) {
        devices_list_unlock();
        LOGD("not find device\n");
        return -1;
    }
    path = g_strdup(device->path);
    devices_list_unlock();

    ret = g_dbus_connection_call_sync(cli.system_conn, BLUEZ_SERVICE,
                path,   FREEDESKTOP_PROPERTIES, "Set",
                g_variant_new("(ssv)", DEVICE_INTERFACE, property_name,
                    g_variant_new("b", value)), NULL, G_DBUS_CALL_FLAGS_NONE,
                DBUS_REPLY_TIMEOUT, NULL, &error);

    g_free(path);

    if (NULL == ret) {
        LOGW ("Error getting object manager client: %s", error->message);
        g_error_free(error);
        return -1;
    }

    g_variant_unref(ret);
    return 0;
}

/*
 * call remote device avrcp method
 * Only support controls (Play, Pause, Stop, Previous, Next)
 * If success return 0, else return -1;
 */
int device_call_avrcp_method(const gchar* bdaddr, const gchar* method)
{
    LOGD("device:%s,value:%d\n", bdaddr, method);

    struct btd_device * device;
    GError *error = NULL;
    GVariant *value;
    gchar *path;

    if (FALSE == BluetoothManage_InitFlag_Get()) {
        LOGW("BluetoothManage Not Init\n");
        return -1;
    }

    if ((0!=g_strcmp0 (method, "Play"))&&
        (0!=g_strcmp0 (method, "Pause"))&&
        (0!=g_strcmp0 (method, "Stop"))&&
        (0!=g_strcmp0 (method, "Previous"))&&
        (0!=g_strcmp0 (method, "Next")))
    {
        LOGD("Invalid method\n");
        return -1;
    }

    devices_list_lock();
    device = devices_list_find_device_by_bdaddr(bdaddr);

    if (NULL == device) {
        devices_list_unlock();
        LOGD("not find device\n");
        return -1;
    }
    path = g_strdup(device->path);
    devices_list_unlock();

    value = g_dbus_connection_call_sync(cli.system_conn, BLUEZ_SERVICE,
                    path,    MEDIA_CONTROL1_INTERFACE,
                    method,   NULL,  NULL,
                    G_DBUS_CALL_FLAGS_NONE, DBUS_REPLY_TIMEOUT,
                    NULL, &error);

    g_free(path);

    if (NULL == value) {
        LOGW ("Error : %s", error->message);
        g_error_free(error);
        return -1;
    }

    g_variant_unref(value);

    return 0;
}


/*
 * Stops the GMainLoop
 */
int BluetoothManagerQuit()
{
    LOGD("\n");

    if (FALSE == BluetoothManage.inited )
    {
        LOGD("BluetoothManage Not init\n");
        return -1;
    }

    if(cli.clientloop){
        g_main_loop_quit(cli.clientloop);
    }

    OfonoManagerQuit();
    BluezManagerQuit();
    stop_agent();

    devices_list_lock();
    devices_list_cleanup();
    devices_list_unlock();


    g_mutex_clear (&(BluetoothManage.m));

    g_object_unref(cli.system_conn);
    g_object_unref(cli.session_conn);

    if (logfp)
        fclose(logfp);
    logfp = NULL;

    BluetoothManage.inited = FALSE;

    return 0;
}

/*
 * Create Bluetooth Manager Thread
 * Note: bluetooth-api shall do BluetoothManageInit() first before call other APIs.
 *          bluetooth-api shall register callback function
 * Returns: 0 - success or other errors
 */
int BluetoothManagerInit() {
    pthread_t thread_id;

    LOGD("\n");

    if (TRUE == BluetoothManage.inited )
    {
        LOGW("BluetoothManage already init\n");
        return -1;
    }

    logfp = fopen("/var/log/BluetoothManager.log", "a+");

    if (NULL == logfp)
    {
        openlog("BluetoothManager", LOG_CONS | LOG_PID, LOG_USER);
        syslog(LOG_WARNING, "BluetoothManager create log file fail\n");
        closelog();
    }

    g_mutex_init(&(BluetoothManage.m));

    pthread_create(&thread_id, NULL, bt_event_loop_thread, NULL);
    //pthread_setname_np(thread_id, "BT_Manager");

    return 0;
}

/*
 * Register Bluetooth Manager Callback function
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

        if (NULL != pstRegisterCallback->binding_device_properties_changed)
        {
            g_RegisterCallback.binding_device_properties_changed =
                pstRegisterCallback->binding_device_properties_changed;
        }

        if (NULL != pstRegisterCallback->binding_request_confirmation)
        {
            g_RegisterCallback.binding_request_confirmation =
                pstRegisterCallback->binding_request_confirmation;
        }
    }
}

GError* setHMIStatus(enum btStates state) {

    gchar *iconString = NULL;
    GDBusConnection *connection;
    GVariant *params = NULL;
    GVariant *message = NULL;
    GError *error = NULL;

    if (state==INACTIVE) iconString = "qrc:/images/Status/HMI_Status_Bluetooth_Inactive-01.png";
    else if (state==ACTIVE) iconString = "qrc:/images/Status/HMI_Status_Bluetooth_On-01.png";
    else iconString = "qrc:/images/Status/HMI_Status_Bluetooth_Inactive-01.png";

    connection = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);

    params = g_variant_new("(is)", HOMESCREEN_BT_ICON_POSITION, iconString);

    message = g_dbus_connection_call_sync(connection, HOMESCREEN_SERVICE,
    HOMESCREEN_ICON_PATH, HOMESCREEN_ICON_INTERFACE, "setStatusIcon", params,
            NULL, G_DBUS_CALL_FLAGS_NONE,
            DBUS_REPLY_TIMEOUT, NULL, &error);

    if (error) {
        printf("error: %s\n", error->message);

        return error;
    } else {
        return NULL;
    }

}


