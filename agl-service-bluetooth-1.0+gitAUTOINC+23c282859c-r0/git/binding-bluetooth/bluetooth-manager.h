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


#ifndef BLUEZ_MANAGER_H
#define BLUEZ_MANAGER_H

#include <glib.h>
#include <gio/gio.h>
#include <glib-object.h>
#include <sys/ioctl.h>

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

//service
#define AGENT_SERVICE               "org.agent"

//remote service
#define BLUEZ_SERVICE               "org.bluez"
#define OFONO_SERVICE               "org.ofono"
#define CLIENT_SERVICE              "org.bluez.obex"

//object path
#define OFONO_MANAGER_PATH          "/"
#define BLUEZ_MANAGER_PATH          "/"
#define AGENT_PATH                  "/org/bluez"
#define ADAPTER_PATH                "/org/bluez/hci0"
#define OBEX_CLIENT_PATH            "/org/bluez/obex"


//interface
#define ADAPTER_INTERFACE           "org.bluez.Adapter1"
#define DEVICE_INTERFACE            "org.bluez.Device1"
#define AGENT_MANAGER_INTERFACE     "org.bluez.AgentManager1"
//#define SERVICE_INTERFACE           "org.bluez.Service"
#define AGENT_INTERFACE             "org.bluez.Agent"

#define CLIENT_INTERFACE            "org.bluez.obex.Client1"
#define TRANSFER_INTERFACE          "org.bluez.obex.Transfer"
#define SESSION_INTERFACE           "org.bluez.obex.Session"
#define OBEX_ERROR_INTERFACE        "org.bluez.obex.Error"
#define BLUEZ_ERROR_INTERFACE       "org.bluez.Error"
#define PBAP_INTERFACE              "org.bluez.obex.PhonebookAccess"
#define MAP_INTERFACE               "org.bluez.obex.MessageAccess"
#define MAP_MSG_INTERFACE           "org.bluez.obex.Message"

#define MEDIA_PLAYER_INTERFACE      "org.bluez.MediaPlayer"
#define MEDIA_PLAYER1_INTERFACE     "org.bluez.MediaPlayer1"
#define MEDIA_FOLDER_INTERFACE      "org.bluez.MediaFolder"
#define MEDIA_ITEM_INTERFACE        "org.bluez.MediaItem"
#define MEDIA_TRANSPORT_INTERFACE   "org.bluez.MediaTransport"
#define MEDIA_TRANSPORT1_INTERFACE  "org.bluez.MediaTransport1"
#define MEDIA_CONTROL1_INTERFACE    "org.bluez.MediaControl1"


#define OFONO_HANDSFREE_INTERFACE               "org.ofono.Handsfree"
#define OFONO_MANAGER_INTERFACE                 "org.ofono.Manager"
#define OFONO_MODEM_INTERFACE                   "org.ofono.Modem"
#define OFONO_VOICECALL_INTERFACE               "org.ofono.VoiceCall"
#define OFONO_VOICECALL_MANAGER_INTERFACE       "org.ofono.VoiceCallManager"
#define OFONO_NETWORK_REGISTRATION_INTERFACE    "org.ofono.NetworkRegistration"
#define OFONO_NETWORK_OPERATOR_INTERFACE        "org.ofono.NetworkOperator"
#define OFONO_CALL_VOLUME_INTERFACE             "org.ofono.CallVolume"

#define FREEDESKTOP_INTROSPECT      "org.freedesktop.DBus.Introspectable"
#define FREEDESKTOP_PROPERTIES      "org.freedesktop.DBus.Properties"
#define FREEDESKTOP_OBJECTMANAGER   "org.freedesktop.DBus.ObjectManager"

#define HOMESCREEN_SERVICE				"org.agl.homescreen"
#define HOMESCREEN_ICON_INTERFACE		"org.agl.statusbar"
#define HOMESCREEN_ICON_PATH			"/StatusBar"
#define HOMESCREEN_BT_ICON_POSITION		1

#define DBUS_REPLY_TIMEOUT (120 * 1000)
#define DBUS_REPLY_TIMEOUT_SHORT (10 * 1000)

#define ERROR_BLUEZ_REJECT "org.bluez.Error.Rejected"
#define ERROR_BLUEZ_CANCELED "org.bluez.Error.Canceled"

#define HCIDEVUP        _IOW('H', 201, int)

#define AF_BLUETOOTH    31
#define BTPROTO_HCI     1

#if 0
void DebugTraceSendMsg(int level, gchar* message);
#else

typedef struct _client
{
    GDBusConnection *system_conn;
    GDBusConnection *session_conn;
    GMainLoop *clientloop;
} Client;

//Bluetooth Device Properties
struct btd_device {
    gchar   *path;
    gchar   *bdaddr;
    gchar   *name;
    gchar   *avrcp_title;
    gchar   *avrcp_artist;
    gchar   *avrcp_status;
    gchar   *transport_state;
    guint32 avrcp_duration;
    guint32 avrcp_position;
    guint16 transport_volume;
    gboolean    paired;
    gboolean    trusted;
    gboolean    connected;
    gboolean    avconnected;
    gboolean    hfpconnected;
    GList   *uuids;
};

typedef struct {
    gboolean inited;
    GMutex m;
    gint watch;
    guint autoconnect;
    GSList * device;
    GSList * priorities;
} stBluetoothManage;

typedef struct tagBinding_RegisterCallback
{
    void (*binding_device_added)(struct btd_device *BDdevice);
    void (*binding_device_removed)(struct btd_device *BDdevice);
    void (*binding_device_properties_changed)(struct btd_device *BDdevice);
    gboolean (*binding_request_confirmation)(const gchar *device, guint passkey);
}Binding_RegisterCallback_t;

enum btStates {INACTIVE, ACTIVE};

void DebugTraceSendMsg(int level, gchar* message);

/* ------ PUBLIC PLUGIN FUNCTIONS --------- */
void BindingAPIRegister(const Binding_RegisterCallback_t* pstRegisterCallback);
int BluetoothManagerInit(void);
int BluetoothManagerQuit(void);

GSList* adapter_get_devices_list() ;
void adapter_devices_list_free(GSList* list) ;

int adapter_set_powered(gboolean value);
int adapter_get_powered(gboolean *value);
//int adapter_set_discoverable(gboolean value);
int adapter_start_discovery();
int adapter_stop_discovery();
int adapter_remove_device(const gchar *addr);
int device_pair(const gchar * addr);
int device_cancelPairing(const gchar * bdaddr);
int device_connect(const gchar *addr, const gchar *uuid);
int device_disconnect(const gchar *addr, const gchar *uuid);
int device_set_property(const gchar * bdaddr, const gchar *property, const gchar *value);
int device_call_avrcp_method(const gchar* device, const gchar* method);
int device_priority_list(void *(object_cb)(void *, gchar *), void *ptr);

int adapter_set_property(const gchar* property, gboolean value) ;

GError* setHMIStatus(enum btStates);

#endif
#endif /* BLUETOOTH_MANAGER_H */


/****************************** The End Of File ******************************/

