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

#ifndef BLUEZ_CLIENT_H
#define BLUEZ_CLIENT_H

#include <glib.h>
#include <gio/gio.h>
#include <glib-object.h>

#include "lib_bluez.h"
#include "bluetooth-manager.h"

//#define BLUEZ_THREAD
//#define BLUEZ_BD_LIST

//Bluetooth Device Properties
struct bt_device {
    gchar   *path;
    gchar   *bdaddr;
    gchar   *name;
    gchar   *alias;
    gboolean    paired;
    gboolean    trusted;
    gboolean    blocked;
    gboolean    connected;
    gboolean    avconnected;
    gboolean    legacypairing;
    gint16      rssi;
    GList   *uuids;
};

typedef struct {
    gboolean inited;
    #ifdef BLUEZ_BD_LIST
    GMutex m;
    GSList * device;
    #endif
    GDBusObjectManager *proxy;
    GDBusConnection *system_conn;
    GDBusConnection *session_conn;
} stBluezManage;

typedef struct tagBluez_RegisterCallback
{
    void (*device_added)(struct bt_device *device);
    void (*device_removed)(const gchar *path);
    void (*device_properties_changed)(const gchar *pObjecPath, const gchar *pInterface, GVariant *value);
}Bluez_RegisterCallback_t;

/* --- PUBLIC FUNCTIONS --- */
void BluezDeviceAPIRegister(const Bluez_RegisterCallback_t* pstRegisterCallback);

int BluezManagerInit(void) ;
int BluezManagerQuit(void) ;

GSList * GetBluezDevicesList(void);
void FreeBluezDevicesList(GSList* list) ;

#endif /* BLUEZ_CLIENT_H */


/****************************** The End Of File ******************************/

