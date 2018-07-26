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

#include "lib_ofono.h"
#include "lib_ofono_modem.h"
#include "ofono-client.h"

#ifdef OFONO_THREAD
static GMainLoop *OfonoLoop = NULL;
#endif

static stOfonoManager OfonoManager = { 0 };
static Ofono_RegisterCallback_t ofono_RegisterCallback = { 0 };


/* ------ LOCAL  FUNCTIONS --------- */
static OFONOMODEMOrgOfonoModem* modem_create_proxy(struct ofono_modem *modem);


/*
 * make a copy of each element
 * And, to entirely free the new btd_device, you could do: modem_free
 */
struct ofono_modem *modem_copy(struct ofono_modem* modem)
{
    struct ofono_modem * temp;

    if (NULL == modem) {
        return NULL;
    }

    temp = g_malloc0(sizeof(struct ofono_modem));
    temp->path = g_strdup(modem->path);

    temp->proxy = g_object_ref (modem->proxy);

    temp->powered = modem->powered;

    return temp;
}

/*
 * Frees all of the memory
 */
static void modem_free(struct ofono_modem* modem)
{

    if (NULL == modem) {
        return ;
    }

    if (modem->path) {
        g_free(modem->path);
        modem->path = NULL;
    }

    if (modem->proxy){
        g_object_unref(modem->proxy);
        modem->proxy = NULL;
    }

    g_free(modem);

}

#if 0
//debug print
void modem_print(struct ofono_modem *modem)
{
    gchar *s;
    g_print("device %p\n",modem);
    g_print("path\t\t:%s\n",modem->path);
    g_print("powered\t\t:%d\n",modem->powered);

}
#endif

static int modem_path_cmp(struct ofono_modem * modem, const gchar* path )
{
    return g_strcmp0 (modem->path, path);
}

static void modem_list_lock(void)
{
    g_mutex_lock(&(OfonoManager.m));
}

static void modem_list_unlock(void)
{
    g_mutex_unlock(&(OfonoManager.m));
}

#if 0
//debug print
void modem_list_print()
{
    GSList * temp = OfonoManager.modem;
    while (temp) {
        struct ofono_modem *modem = temp->data;
        temp = temp->next;
        g_print("----------------------------------------\n");
        modem_print(modem);
    }
    g_print("----------------------------------------\n");
}
#endif


/*
 * remove all the devices
*/
static void modem_list_cleanup()
{
    LOGD("\n");
    GSList * temp = OfonoManager.modem;
    while (temp) {
        struct ofono_modem *modem = temp->data;
        temp = temp->next;

        OfonoManager.modem = g_slist_remove_all(OfonoManager.modem,
                modem);

        modem_free(modem);
    }
}

/*
 * search ofono modem by path
 * Returns the first found btd_device or NULL if it is not found
 */
static  struct ofono_modem *modem_list_find_modem_by_path(const char* path)
{
    //LOGD("path%s\n",path);
    GSList * temp = NULL;

    temp = g_slist_find_custom (OfonoManager.modem, path,
                                (GCompareFunc)modem_path_cmp);

    if (temp) {
        return temp->data;
    }

    return NULL;
}

static void on_modem_property_changed (OFONOMODEMOrgOfonoModem* object,
                                        gchar* property,
                                        GVariant *value,
                                        gpointer userdata)
{
#ifdef _DEBUG_PRINT_DBUS
    gchar *s;

    g_print ("%s\n",property);
    s = g_variant_print (value, TRUE);
    g_print ("  %s\n",  s);
    g_free (s);
#endif

    struct ofono_modem *modem;


    if (NULL==property || NULL==value || NULL==userdata){
        LOGD(" receive null data\n");
        return;
    }

    modem = userdata;

    if (0 == g_strcmp0(property, "Powered"))
    {

        GVariant *temp = g_variant_get_variant (value);
        //g_print ("update\n");
        gboolean new_value;
        //gboolean old_value;
        g_variant_get(temp, "b", &new_value );
        g_variant_unref(temp);
        LOGD("Powered %d\n",new_value);
        //old_value = modem->powered;
        modem->powered = new_value;

        if (NULL != ofono_RegisterCallback.modem_properties_changed)
        {
            ofono_RegisterCallback.modem_properties_changed(modem);
        }
    }


}

static void on_modem_added (OFONOOrgOfonoManager* object,
                            gchar* path,
                            GVariant *value,
                            gpointer userdata)
{
#ifdef _DEBUG_PRINT_DBUS
    gchar *s;

    g_print ("on_modom_added\n");
    g_print ("%s\n",path);
    s = g_variant_print (value, TRUE);
    g_print ("  %s\n",  s);
    g_free (s);
#endif


    GError *error = NULL;

    if (NULL == path)
        return;

    struct ofono_modem *modem = g_malloc0(sizeof(struct ofono_modem));

    LOGD("new modem path:%s,%p\n", path, modem);

    modem->path = g_strdup(path);
    modem->proxy = modem_create_proxy(modem);



    GVariant *property_value = NULL;
    g_variant_lookup(value, "Powered",
                     "*", &property_value);

    if (property_value)
    {
        gboolean bValue = FALSE;
        g_variant_get(property_value, "b", &bValue );
        modem->powered = bValue;

        g_variant_unref (property_value);
    }

    modem_list_lock();
    OfonoManager.modem = g_slist_prepend(OfonoManager.modem, modem);
    modem_list_unlock();

    if (NULL != ofono_RegisterCallback.modem_added){
        ofono_RegisterCallback.modem_added(modem);
    }
}

static void on_modem_removed (OFONOOrgOfonoManager * object,
                                gchar* path,
                                gpointer userdata)
{
#ifdef _DEBUG_PRINT_DBUS
    gchar *s;
    g_print ("on_modem_removed\n");
    g_print ("%s\n",path);
#endif

    struct ofono_modem *modem = NULL;

    modem_list_lock();

    modem = modem_list_find_modem_by_path(path);

    LOGD("remove modem path:%s,%p\n", path, modem);

    if (modem){

        OfonoManager.modem = g_slist_remove_all(OfonoManager.modem,
                                    modem);

        if (NULL != ofono_RegisterCallback.modem_removed)
        {
            ofono_RegisterCallback.modem_removed(modem);
        }

        modem_free(modem);
    }

    modem_list_unlock();

}


static OFONOMODEMOrgOfonoModem* modem_create_proxy(struct ofono_modem *modem)
{
    GError *error = NULL;

    if (NULL == modem)
    {
        return NULL;
    }

    if (NULL == modem->path)
    {
        return NULL;
    }

    modem->proxy = ofono_modem_org_ofono_modem_proxy_new_for_bus_sync (
        G_BUS_TYPE_SYSTEM, G_DBUS_PROXY_FLAGS_NONE, OFONO_SERVICE,
        modem->path, NULL, &error);

    if (error)
    {
        LOGW ("Error : %s\n", error->message);
        g_error_free(error);
        return NULL;
    }

    g_signal_connect (modem->proxy,
                      "property_changed",
                      G_CALLBACK (on_modem_property_changed),
                      modem);

    return modem->proxy;

}


/*
 * Force Update the modem list
 * Call <method>GetModems
 * Returns: 0 - success or other errors
 */
int modem_list_update() {
    LOGD("\n");

    GError *error = NULL;
    GVariant *result = NULL;
    GSList *newDeviceList = NULL;


    result = g_dbus_connection_call_sync(OfonoManager.system_conn,
            OFONO_SERVICE, OFONO_MANAGER_PATH, OFONO_MANAGER_INTERFACE,
            "GetModems", NULL, NULL,
            G_DBUS_CALL_FLAGS_NONE, DBUS_REPLY_TIMEOUT, NULL, &error);

    if (error) {
        LOGW ("Error : %s\n", error->message);
        g_error_free(error);
        return -1;
    }

    GVariant *ArrayValue = NULL;
    GVariantIter *ArrayValueIter;
    GVariant *Value = NULL;

    g_variant_get(result, "(*)", &ArrayValue);

    g_variant_get(ArrayValue, "a*", &ArrayValueIter);
    while (g_variant_iter_loop(ArrayValueIter, "*", &Value)) {

        GVariantIter dbus_object_iter;
        GVariant *dbusObjecPath;
        GVariant *dbusObjecProperties;

        gchar *pObjecPath = NULL;
        struct ofono_modem *modem = NULL;

        g_variant_iter_init(&dbus_object_iter, Value);

        //1st : DBus Object Path
        dbusObjecPath = g_variant_iter_next_value(&dbus_object_iter);

        g_variant_get(dbusObjecPath, "o", &pObjecPath);

        LOGD("object path %s\n",pObjecPath);

          //ObjectPath is /hfp/org/bluez/hci0/dev_xx_xx_xx_xx_xx_xx
        if ((41 != strlen(pObjecPath))
            || (NULL == g_strstr_len(pObjecPath, 23,
                                      "/hfp"ADAPTER_PATH"/dev"))) {
            g_free(pObjecPath);
            pObjecPath = NULL;
            g_variant_unref(dbusObjecPath);
            continue;
        }

        modem = g_malloc0(sizeof(struct ofono_modem));
        modem->path = g_strdup(pObjecPath);
        g_free(pObjecPath);
        pObjecPath = NULL;
        g_variant_unref(dbusObjecPath);

        LOGD("Found new device%s\n",modem->path );

        modem->proxy = modem_create_proxy(modem);

        //2nd : DBus Interfaces under Object Path
        dbusObjecProperties = g_variant_iter_next_value(&dbus_object_iter);

        GVariant *property_value = NULL;
        g_variant_lookup(dbusObjecProperties, "Powered",
                         "*", &property_value);

        if (property_value)
        {
            gboolean bValue = FALSE;
            g_variant_get(property_value, "b", &bValue );
            modem->powered = bValue;

            g_variant_unref (property_value);
        }

        g_variant_unref(dbusObjecProperties);

        //Save device to newDeviceList
        newDeviceList = g_slist_prepend(newDeviceList, modem);

    }

    g_variant_iter_free(ArrayValueIter);
    g_variant_unref(ArrayValue);

    g_variant_unref(result);


    //Force update device, so clean first
    modem_list_lock();

    modem_list_cleanup();

    OfonoManager.modem = newDeviceList;

    modem_list_unlock();

    return 0;
}


/*
 * init cli dbus connection
 * Returns: 0 - success or other errors
 */
static int ofono_manager_connect_to_dbus(void)
{
    GError *error = NULL;

    OfonoManager.system_conn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);

    if (error)
    {
        LOGE("errr:%s",error->message);
        g_error_free(error);

        return -1;
    }

    OfonoManager.proxy = ofono_org_ofono_manager_proxy_new_for_bus_sync(
        G_BUS_TYPE_SYSTEM, G_DBUS_PROXY_FLAGS_NONE, OFONO_SERVICE,
        OFONO_MANAGER_PATH, NULL, &error);

    if (error) {
        LOGW("Create Ofono manager client fail\n");
        LOGW("Error:%s\n", error->message);
        g_error_free(error);

        g_object_unref(OfonoManager.system_conn);
        return -1;
    }

    return 0;
}

/*
 * register callback function
 * Returns: 0 - success or other errors
 */
static int ofono_manager_register_callback(void)
{

    g_signal_connect (OfonoManager.proxy,
                      "modem-added",
                      G_CALLBACK (on_modem_added),
                      NULL);

    g_signal_connect (OfonoManager.proxy,
                      "modem-removed",
                      G_CALLBACK (on_modem_removed),
                      NULL);

    return 0;
}

/*
 * init dbus and register callback
 */
static int ofono_mamager_init(void)
{
    int ret = 0;
    LOGD("\n");
    ret = ofono_manager_connect_to_dbus();

    if (0 != ret){
        LOGW("init fail\n");
        return -1;
    }

    g_mutex_init(&(OfonoManager.m));

    ofono_manager_register_callback();
    modem_list_update();

    OfonoManager.inited = TRUE;

    return 0;
}

#ifdef AGENT_THREAD
/*
 * Ofono Manager Thread
 * register callback function and create a new GMainLoop structure
 */
static void *ofono_event_loop_thread()
{
    int ret = 0;

    OfonoLoop = g_main_loop_new(NULL, FALSE);;

    ret = ofono_mamager_init();

    if (0 == ret){
        LOGD("g_main_loop_run\n");
        g_main_loop_run(OfonoLoop);

    }

    g_main_loop_unref(OfonoLoop);
    LOGD("exit...\n");
}
#endif

/* --- PUBLIC FUNCTIONS --- */

/*
 * Get the ofono modem "Powered" property
 */
gboolean getOfonoModemPoweredByPath (gchar* path)
{
    struct ofono_modem *modem = NULL;
    gboolean powered = FALSE;

    modem_list_lock();

    modem = modem_list_find_modem_by_path(path);

    if (modem){
        powered = modem->powered;
    }

    LOGD("get modem %p by path:%s,%d\n", modem, path, powered);

    modem_list_unlock();

    return powered;
}


/*
 * Init the ofono manager
 */
int OfonoManagerInit(void)
{
    int ret = 0;

    LOGD("\n");

    if (TRUE == OfonoManager.inited)
    {
        LOGW("Ofono Manager is already inited\n");
        return -1;

    }

#ifdef OFONO_THREAD
    pthread_t thread_id;

    pthread_create(&thread_id, NULL, ofono_event_loop_thread, NULL);
    pthread_setname_np(thread_id, "ofono_manager");

#else

    ret = ofono_mamager_init();

#endif

    return ret;
}

/*
 * Quit the ofono manager
 */
int OfonoManagerQuit(void)
{
    LOGD("\n");

    if (TRUE != OfonoManager.inited)
    {
        LOGW("Ofono Manager is not inited\n");
        return -1;

    }
#ifdef OFONO_THREAD
    g_main_loop_quit(OfonoLoop);
#endif

    memset(&ofono_RegisterCallback, 0, sizeof(Ofono_RegisterCallback_t));

    g_object_unref(OfonoManager.proxy);

    modem_list_lock();
    modem_list_cleanup();
    modem_list_unlock();

    g_mutex_clear (&(OfonoManager.m));

    g_object_unref(OfonoManager.system_conn);

    OfonoManager.inited = FALSE;

    return 0;

}

/*
 * Register ofono Callback function
 */
void OfonoModemAPIRegister(const Ofono_RegisterCallback_t* pstRegisterCallback)
{
    LOGD("\n");

    if (NULL != pstRegisterCallback)
    {
        if (NULL != pstRegisterCallback->modem_added)
        {
            ofono_RegisterCallback.modem_added =
                pstRegisterCallback->modem_added;
        }

        if (NULL != pstRegisterCallback->modem_removed)
        {
            ofono_RegisterCallback.modem_removed =
                pstRegisterCallback->modem_removed;
        }

        if (NULL != pstRegisterCallback->modem_properties_changed)
        {
            ofono_RegisterCallback.modem_properties_changed =
                pstRegisterCallback->modem_properties_changed;
        }

    }

}

