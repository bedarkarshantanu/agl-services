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
#include <glib.h>
#include <gio/gio.h>
#include <glib-object.h>

#include "lib_bluez.h"
#include "bluez-client.h"

#ifdef BLUEZ_THREAD
static GMainLoop *BluezLoop;
#endif
static Bluez_RegisterCallback_t bluez_RegisterCallback = { 0 };
static stBluezManage BluezManage = { 0 };


/* ------ LOCAL  FUNCTIONS --------- */

/*
 * make a copy of each element
 * And, to entirely free the new btd_device, you could do: device_free
 */
static struct bt_device *bluez_device_copy(struct bt_device* device)
{
    struct bt_device * temp;

    if (NULL == device) {
        return NULL;
    }

    temp = g_malloc0(sizeof(struct bt_device));
    temp->path = g_strdup(device->path);
    temp->bdaddr = g_strdup(device->bdaddr);
    temp->name = g_strdup(device->name);
    temp->alias = g_strdup(device->alias);
    temp->paired = device->paired;
    temp->trusted = device->trusted;
    temp->blocked = device->blocked;
    temp->connected = device->connected;
    temp->avconnected = device->avconnected;
    temp->legacypairing = device->legacypairing;
    temp->rssi = device->rssi;
    temp->uuids = g_list_copy(device->uuids);

    return temp;
}

/*
 * Frees all of the memory
 */
static void bluez_device_free(struct bt_device* device)
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
    if (device->alias) {
        D_PRINTF("alias:%s\n",device->alias);
        g_free(device->alias);
        device->alias = NULL;
    }

    if (device->uuids){
        D_PRINTF("uuids xxx\n");
        g_list_free_full(device->uuids, g_free);
        device->uuids = NULL;
    }

    g_free(device);

}

#ifdef BLUEZ_BD_LIST

static void bluez_devices_list_lock(void)
{
    g_mutex_lock(&(BluezManage.m));
}

static void bluez_devices_list_unlock(void)
{
    g_mutex_unlock(&(BluezManage.m));
}

static int bluez_device_path_cmp(struct bt_device * device, const gchar* pPath )
{
    return g_strcmp0 (device->path, pPath);
}

/*
 * search device by path
 * Returns the first found btd_device or NULL if it is not found
 */
static struct bt_device *bluez_devices_list_find_device_by_path(const gchar* pPath)
{
    GSList * temp;

    temp = g_slist_find_custom (BluezManage.device, pPath,
                                (GCompareFunc)bluez_device_path_cmp);

    if (temp) {
        return temp->data;
    }

    return NULL;

}


static int bluez_device_bdaddr_cmp(struct bt_device * device, const gchar* pBDaddr )
{
    return g_strcmp0 (device->bdaddr, pBDaddr);
}

/*
 * search device by path
 * Returns the first found btd_device or NULL if it is not found
 */
static struct
bt_device *bluez_devices_list_find_device_by_bdaddr(const gchar* pBDaddr)
{
    GSList * temp;

    temp = g_slist_find_custom (BluezManage.device, pBDaddr,
                                (GCompareFunc)bluez_device_bdaddr_cmp);

    if (temp) {
        return temp->data;
    }

    return NULL;

}

/*
 * remove all the devices
 */
static void bluez_devices_list_cleanup()
{
    LOGD("\n");
    GSList * temp = BluezManage.device;
    while (temp) {
        struct bt_device *BDdevice = temp->data;
        temp = temp->next;

        BluezManage.device = g_slist_remove_all(BluezManage.device,
                BDdevice);

        bluez_device_free(BDdevice);
    }
}

/*
 * Get the copy of device list.
 */
GSList* bluez_devices_list_copy()
{
    GSList* tmp;
    bluez_devices_list_lock();
    tmp = g_slist_copy_deep (BluezManage.device,
                                        (GCopyFunc)bluez_device_copy, NULL);
    bluez_devices_list_unlock();
    return tmp;
}

#endif

/*
 * free device list.
 */
void bluez_devices_list_free(GSList* list)
{
    if (NULL != list)
        g_slist_free_full(list,(GDestroyNotify)bluez_device_free);

}

/*
 * update device from Interfcace org.bluez.Device1 properties
 */
static int
bluez_device1_properties_update(struct bt_device *device, GVariant *value)
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
        gint16 value_n  =  0;//n gint16
        //guint16 value_q  =  0;//q guint16
        //gint32 value_i  =  0;//i gint32
        //guint32 value_u  =  0;//u guint32
        //gint64 value_x  =  0;//x gint64
        //guint64 value_t  =  0;//t guint64
        //gint32 value_h  = 0;//h gint32
        //gdouble value_d = 0.0;//d gdouble
        gchar *str;

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

        }else if (0==g_strcmp0(key,"Alias")) {
            g_variant_get(subValue, "s", &str );
            D_PRINTF("Alias %s\n",str);

            if (device->alias)
                g_free (device->alias);

            device->alias = g_strdup(str);

            g_free (str);
            str = NULL;
        }else if (0==g_strcmp0(key,"LegacyPairing")) {
            g_variant_get(subValue, "b", &value_b );
            D_PRINTF("LegacyPairing %d\n",value_b);
            device->legacypairing = value_b;

        }else if (0==g_strcmp0(key,"Paired")) {
            g_variant_get(subValue, "b", &value_b );
            D_PRINTF("Paired %d\n",value_b);
            device->paired = value_b;

        }else if (0==g_strcmp0(key,"Trusted")) {
            g_variant_get(subValue, "b", &value_b );
            D_PRINTF("Trusted %d\n",value_b);
            device->trusted = value_b;

        }else if (0==g_strcmp0(key,"Blocked")) {
            g_variant_get(subValue, "b", &value_b );
            D_PRINTF("Blocked %d\n",value_b);
            device->blocked = value_b;

        }else if (0==g_strcmp0(key,"Connected")) {
            g_variant_get(subValue, "b", &value_b );
            D_PRINTF("Connected %d\n",value_b);
            device->connected = value_b;

        }else if (0==g_strcmp0(key,"RSSI")) {
            g_variant_get(subValue, "n", &value_n );
            D_PRINTF("RSSI %d\n",value_n);
            device->rssi = value_n;

        }else if (0==g_strcmp0(key,"UUIDs")) {
            GVariantIter iter;
            gchar *val;

            //g_print ("type '%s'\n", g_variant_get_type_string (subValue));
            if (device->uuids) {
                g_list_free_full(device->uuids, g_free);
            }

            g_variant_iter_init (&iter, subValue);
            while (g_variant_iter_next (&iter, "s", &val))
            {
                device->uuids = g_list_append(device->uuids, g_strdup(val));
            }
        }
    }

    return 0;

}

/*
 * update device from Interfcace org.bluez.MediaControl1 properties
 */
static int
bluez_mediacontrol1_properties_update(struct bt_device *device, GVariant *value)
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

        gboolean value_b = FALSE;//b gboolean
        gchar *str;

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
 * Get the device list
 * Call <method>GetManagedObjects
 * Returns: 0 - success or other errors
 */
static GSList * bluez_get_devices_list() {
    LOGD("\n");

    GError *error = NULL;
    GVariant *result = NULL;
    GSList *newDeviceList = NULL;

    result = g_dbus_connection_call_sync(BluezManage.system_conn,
            BLUEZ_SERVICE, BLUEZ_MANAGER_PATH, FREEDESKTOP_OBJECTMANAGER,
            "GetManagedObjects", NULL, NULL,
            G_DBUS_CALL_FLAGS_NONE, DBUS_REPLY_TIMEOUT, NULL, &error);

    if (error) {
        LOGW ("Error : %s\n", error->message);
        g_error_free(error);
        return NULL;
    }

    GVariant *ArrayValue = NULL;
    GVariantIter *ArrayValueIter;
    GVariant *Value = NULL;

    g_variant_get(result, "(*)", &ArrayValue);

    g_variant_get(ArrayValue, "a*", &ArrayValueIter);
    while (g_variant_iter_loop(ArrayValueIter, "*", &Value)) {

        GVariantIter dbus_object_iter;
        GVariant *dbusObjecPath;
        GVariant *dbusObjecInterfaces;

        gchar *pObjecPath = NULL;
        struct bt_device *device = NULL;

        g_variant_iter_init(&dbus_object_iter, Value);

        //1st : DBus Object Path
        dbusObjecPath = g_variant_iter_next_value(&dbus_object_iter);

        g_variant_get(dbusObjecPath, "o", &pObjecPath);

        LOGD("object path %s\n",pObjecPath);
          //ObjectPath is /org/bluez/hci0/dev_xx_xx_xx_xx_xx_xx
        if ((37 != strlen(pObjecPath))
            || (NULL == g_strrstr_len(pObjecPath, 19,
                                      ADAPTER_PATH"/dev"))) {
            g_free(pObjecPath);
            pObjecPath = NULL;
            g_variant_unref(dbusObjecPath);
            continue;
        }
        device = g_malloc0(sizeof(struct bt_device));
        device->path = g_strdup(pObjecPath);
        g_free(pObjecPath);
        pObjecPath = NULL;
        g_variant_unref(dbusObjecPath);

        LOGD("Found new device%s\n",device->path );

        //2nd : DBus Interfaces under Object Path
        dbusObjecInterfaces = g_variant_iter_next_value(&dbus_object_iter);

        GVariant *interfaces_value = NULL;
        g_variant_lookup(dbusObjecInterfaces, DEVICE_INTERFACE,
                         "*", &interfaces_value);

        if (interfaces_value)
        {
            bluez_device1_properties_update(device, interfaces_value);

            g_variant_unref (interfaces_value);
            interfaces_value = NULL;
        }

        g_variant_lookup(dbusObjecInterfaces, MEDIA_CONTROL1_INTERFACE,
                         "*", &interfaces_value);

        if (interfaces_value)
        {
            bluez_mediacontrol1_properties_update(device, interfaces_value);

            g_variant_unref (interfaces_value);
            interfaces_value = NULL;
        }

        g_variant_unref(dbusObjecInterfaces);

        //newDeviceList = g_slist_prepend(newDeviceList, device);
        newDeviceList = g_slist_append(newDeviceList, device);


    }

    g_variant_iter_free(ArrayValueIter);
    g_variant_unref(ArrayValue);

    g_variant_unref(result);

    return newDeviceList;
}



/*
 * notify::name-owner callback function
 */
static void on_notify_name_owner (GObject       *object,
                                  GParamSpec    *pspec,
                                  gpointer       user_data)
{
#ifdef _DEBUG_PRINT_DBUS
  GDBusObjectManagerClient *manager = G_DBUS_OBJECT_MANAGER_CLIENT (object);
  gchar *name_owner;

  name_owner = g_dbus_object_manager_client_get_name_owner (manager);
  g_print ("name-owner: %s\n", name_owner);
  g_free (name_owner);
#endif
}

/*
 * object_added callback function
 */
static void on_object_added (GDBusObjectManager *manager,
                             GDBusObject        *object,
                             gpointer            user_data)
{
#ifdef _DEBUG_PRINT_DBUS
    gchar *owner;
    owner = g_dbus_object_manager_client_get_name_owner (
                G_DBUS_OBJECT_MANAGER_CLIENT (manager));
    g_print ("Added object at %s (owner %s)\n",
                g_dbus_object_get_object_path (object), owner);
    g_free (owner);
#endif


    const gchar *dbusObjecPath;
    GError *error = NULL;
    GVariant *value = NULL;
    struct bt_device *device;

    dbusObjecPath = g_dbus_object_get_object_path (object);

    LOGD("%s\n", dbusObjecPath);

    //ObjectPath is /org/bluez/hci0/dev_xx_xx_xx_xx_xx_xx
    if ((37 != strlen(dbusObjecPath))
        || (NULL == g_strrstr_len(dbusObjecPath, 19,ADAPTER_PATH"/dev"))) {
        return;
    }

    device = g_malloc0(sizeof(struct bt_device));
    device->path = g_strdup(dbusObjecPath);

    value = g_dbus_connection_call_sync(BluezManage.system_conn, BLUEZ_SERVICE,
                dbusObjecPath, FREEDESKTOP_PROPERTIES,
                "GetAll",      g_variant_new("(s)", DEVICE_INTERFACE),
                NULL,          G_DBUS_CALL_FLAGS_NONE,
                DBUS_REPLY_TIMEOUT, NULL, &error);

    if (error) {
        LOGW ("Error : %s\n", error->message);
        g_error_free(error);
        g_free(device->path);
        g_free(device);
        return;
    }

    if (value) {
        GVariant *subValue;
        g_variant_get(value, "(*)", &subValue);

        bluez_device1_properties_update(device, subValue);

        g_variant_unref (subValue);
        g_variant_unref(value);
    }

#ifdef BLUEZ_BD_LIST

    bluez_devices_list_lock();

    //BluezManage.device = g_slist_prepend(BluezManage.device, device);
    BluezManage.device = g_slist_append(BluezManage.device, device);

    if (NULL != bluez_RegisterCallback.device_added)
    {
        bluez_RegisterCallback.device_added(device);
    }

    bluez_devices_list_unlock();
#else

    if (NULL != bluez_RegisterCallback.device_added)
    {
        bluez_RegisterCallback.device_added(device);
    }
    bluez_device_free(device);

#endif
}

/*
 * object-removed callback function
 */
static void on_object_removed (GDBusObjectManager *manager,
                               GDBusObject        *object,
                               gpointer            user_data)
{
#ifdef _DEBUG_PRINT_DBUS
  gchar *owner;

  owner = g_dbus_object_manager_client_get_name_owner (
                G_DBUS_OBJECT_MANAGER_CLIENT (manager));
  g_print ("Removed object at %s (owner %s)\n",
                g_dbus_object_get_object_path (object), owner);
  g_free (owner);
#endif

    const gchar *dbusObjecPath;
    //int ret;


    dbusObjecPath = g_dbus_object_get_object_path (object);

    if ((37 != strlen(dbusObjecPath))
        || (NULL == g_strrstr_len(dbusObjecPath, 19,ADAPTER_PATH"/dev"))) {
        return;
    }

    if (NULL != bluez_RegisterCallback.device_removed)
    {
        bluez_RegisterCallback.device_removed(dbusObjecPath);
    }
    LOGD("%s\n", dbusObjecPath);
#ifdef BLUEZ_BD_LIST
    struct bt_device *device;

    bluez_devices_list_lock();

    device = bluez_devices_list_find_device_by_path(dbusObjecPath);

    if (device) {
        LOGD("Path      :%s.\n", dbusObjecPath);
        BluezManage.device = g_slist_remove_all(BluezManage.device,
                                    device);

        bluez_device_free(device);
    }

    bluez_devices_list_unlock();
#endif

}

/*
 * BLUEZ interface-proxy-properties-changed callback function
 */
static void
on_interface_proxy_properties_changed (GDBusObjectManagerClient *manager,
                                GDBusObjectProxy     *object_proxy,
                                GDBusProxy           *interface_proxy,
                                GVariant             *changed_properties,
                                const gchar *const   *invalidated_properties,
                                gpointer              user_data)
{
    const gchar *pObjecPath;
    const gchar *pInterface;

    pObjecPath = g_dbus_object_get_object_path (G_DBUS_OBJECT (object_proxy));
    pInterface = g_dbus_proxy_get_interface_name  (interface_proxy);

#ifdef _DEBUG_PRINT_DBUS
    gchar *s;
    g_print ("Path:%s, Interface:%s\n",pObjecPath, pInterface);
    g_print ("type '%s'\n", g_variant_get_type_string (changed_properties));
    s = g_variant_print (changed_properties, TRUE);
    g_print (" %s\n", s);
    g_free (s);
#endif

    //ObjectPath is /org/bluez/hci0/dev_xx_xx_xx_xx_xx_xx

    LOGD("%s\n",pObjecPath);

    if( (0 == g_strcmp0(pInterface, DEVICE_INTERFACE)) ||
        (0 == g_strcmp0(pInterface, MEDIA_CONTROL1_INTERFACE)) ||
        (0 == g_strcmp0(pInterface, MEDIA_PLAYER1_INTERFACE)) ||
        (0 == g_strcmp0(pInterface, MEDIA_TRANSPORT1_INTERFACE))) {

        if (bluez_RegisterCallback.device_properties_changed)
            bluez_RegisterCallback.device_properties_changed(pObjecPath,
                                            pInterface, changed_properties);

    }

#ifdef BLUEZ_BD_LIST
    struct bt_device *device;

    if (0 == g_strcmp0(pInterface, DEVICE_INTERFACE)) {

        bluez_devices_list_lock();

        device = bluez_devices_list_find_device_by_path(pObjecPath);

        bluez_device1_properties_update(device, changed_properties);

        bluez_devices_list_unlock();

    } else if (0 == g_strcmp0(pInterface, MEDIA_CONTROL1_INTERFACE)) {

        bluez_devices_list_lock();

        device = bluez_devices_list_find_device_by_path(pObjecPath);

        bluez_mediacontrol1_properties_update(device, changed_properties);

        bluez_devices_list_unlock();
    }
#endif
}

/*
 * init cli dbus connection
 * Returns: 0 - success or other errors
 */
static int bluez_manager_connect_to_dbus(void)
{
    GError *error = NULL;

    BluezManage.system_conn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);

    if (error) {
        LOGE("Create System GDBusconnection fail\n");
        LOGE("Error:%s\n", error->message);
        g_error_free(error);

        return -1;
    }

    BluezManage.session_conn = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);

    if (error) {
        LOGE("Create Session GDBusconnection fail\n");
        LOGE("Error:%s\n", error->message);
        g_error_free(error);

        g_object_unref(BluezManage.system_conn);

        return -1;
    }

    BluezManage.proxy = bluez_object_manager_client_new_for_bus_sync(
        G_BUS_TYPE_SYSTEM, G_DBUS_PROXY_FLAGS_NONE, BLUEZ_SERVICE,
        BLUEZ_MANAGER_PATH, NULL, &error);

    if (error) {
        LOGE("Create Bluez manager client fail\n");
        LOGE("Error:%s\n", error->message);
        g_error_free(error);

        g_object_unref(BluezManage.system_conn);
        g_object_unref(BluezManage.session_conn);
        return -1;
    }

    return 0;
}


/*
 * register dbus callback function
 * Returns: 0 - success or other errors
 */
static int bluez_manager_register_callback(void)
{

    g_signal_connect (BluezManage.proxy,
                      "notify::name-owner",
                      G_CALLBACK (on_notify_name_owner),
                      NULL);

    g_signal_connect (BluezManage.proxy,
                      "object-added",
                      G_CALLBACK (on_object_added),
                      NULL);

    g_signal_connect (BluezManage.proxy,
                      "object-removed",
                      G_CALLBACK (on_object_removed),
                      NULL);

    g_signal_connect (BluezManage.proxy,
                      "interface-proxy-properties-changed",
                      G_CALLBACK (on_interface_proxy_properties_changed),
                      NULL);

    return 0;
}

/*
 * init bluez client
 * Returns: 0 - success or other errors
 */
static int bluez_manager_int()
{
    int ret = 0;

    LOGD("\n");

    ret = bluez_manager_connect_to_dbus();

    if (ret){
        LOGE("Init Fail\n");
        return -1;
    }

    bluez_manager_register_callback();

#ifdef BLUEZ_BD_LIST

    g_mutex_init(&(BluezManage.m));

    BluezManage.device = bluez_get_devices_list();

#endif

    BluezManage.inited = TRUE;

    return 0;

}

#ifdef BLUEZ_THREAD
/*
 * Bluetooth Manager Thread
 * register callback function and create a new GMainLoop structure
 */
static void *bluez_event_loop_thread()
{
    int ret = 0;

    BluezLoop = g_main_loop_new(NULL, FALSE);;

    ret = bluez_manager_int();

    if (0 == ret){
        LOGD("g_main_loop_run\n");
        g_main_loop_run(BluezLoop);
    }

    g_main_loop_unref(BluezLoop);
    LOGD("exit...\n");
}
#endif


/* ------ PUBLIC FUNCTIONS --------- */


/*
 * Get the device list
 * The list should free by FreeBluezDevicesList()
 */
GSList * GetBluezDevicesList(void)
{
    LOGD("\n");

    GSList* newDeviceList = NULL;

    if (TRUE != BluezManage.inited)
    {
        LOGD("Bluez Manager is not inited\n");
        return NULL;
    }

#ifdef BLUEZ_BD_LIST
    newDeviceList = bluez_devices_list_copy();
#else
    newDeviceList = bluez_get_devices_list();
#endif
    return newDeviceList;
}

/*
 * free device list.
 */
void FreeBluezDevicesList(GSList* list)
{
    bluez_devices_list_free(list);
}

/*
 * Stops the GMainLoop
 */
int BluezManagerQuit(void)
{
    LOGD("\n");

    if (FALSE == BluezManage.inited)
    {
        LOGD("Bluez Manager is not inited\n");
        return -1;
    }

#ifdef BLUEZ_THREAD
    g_main_loop_quit(BluezLoop);
#endif

    memset(&bluez_RegisterCallback, 0, sizeof(Bluez_RegisterCallback_t));

    g_object_unref(BluezManage.proxy);

#ifdef BLUEZ_BD_LIST
    bluez_devices_list_lock();
    bluez_devices_list_cleanup();
    bluez_devices_list_unlock();

    g_mutex_clear (&(BluezManage.m));
#endif

    g_object_unref(BluezManage.system_conn);
    g_object_unref(BluezManage.session_conn);

    BluezManage.inited = FALSE;

    return 0;
}

/*
 * Init Bluez Manager
 */
int BluezManagerInit()
{
    LOGD("\n");
    int ret = 0;

    
    if (TRUE == BluezManage.inited)
    {
        LOGD("Bluez Manager is already inited\n");
        return -1;
    }

#ifdef BLUEZ_THREAD

    pthread_t thread_id;
    pthread_create(&thread_id, NULL, bluez_event_loop_thread, NULL);
    pthread_setname_np(thread_id, "Bluez_Manage");

#else

    ret = bluez_manager_int();
#endif

    return ret;
}

/*
 * Register Bluez Manager Callback function
 */
void BluezDeviceAPIRegister(const Bluez_RegisterCallback_t* pstRegisterCallback)
{
    if (NULL != pstRegisterCallback)
    {
        if (NULL != pstRegisterCallback->device_added)
        {
            bluez_RegisterCallback.device_added =
                pstRegisterCallback->device_added;
        }

        if (NULL != pstRegisterCallback->device_removed)
        {
            bluez_RegisterCallback.device_removed =
                pstRegisterCallback->device_removed;
        }

        if (NULL != pstRegisterCallback->device_properties_changed)
        {
            bluez_RegisterCallback.device_properties_changed =
                pstRegisterCallback->device_properties_changed;
        }
    }
}



