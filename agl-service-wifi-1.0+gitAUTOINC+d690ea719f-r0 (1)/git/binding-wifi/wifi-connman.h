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

#define _GNU_SOURCE

//#include <dlog.h>
#include <glib.h>
#include <stdlib.h>
#include <gio/gio.h>
#include <glib-object.h>

#define AFB_BINDING_VERSION 2
#include <afb/afb-binding.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Maximum Profile Count */
#define CONNMAN_MAX_BUFLEN 512

#define CONNMAN_STATE_STRLEN 16

#define CONNMAN_SERVICE                 "net.connman"
#define CONNMAN_MANAGER_INTERFACE		CONNMAN_SERVICE ".Manager"
#define CONNMAN_TECHNOLOGY_INTERFACE	CONNMAN_SERVICE ".Technology"
#define CONNMAN_SERVICE_INTERFACE		CONNMAN_SERVICE ".Service"
#define CONNMAN_PROFILE_INTERFACE		CONNMAN_SERVICE ".Profile"
#define CONNMAN_COUNTER_INTERFACE		CONNMAN_SERVICE ".Counter"
#define CONNMAN_ERROR_INTERFACE			CONNMAN_SERVICE ".Error"
#define CONNMAN_AGENT_INTERFACE			CONNMAN_SERVICE ".Agent"

#define CONNMAN_MANAGER_PATH			"/"
#define CONNMAN_PATH					"/net/connman"
#define CONNMAN_TECHNOLOGY_PATH			"/net/connman/technology/wifi"

/** ConnMan technology and profile prefixes for ConnMan 0.78 */

#define CONNMAN_WIFI_TECHNOLOGY_PREFIX			CONNMAN_PATH "/technology/wifi"
#define CONNMAN_WIFI_SERVICE_PROFILE_PREFIX		CONNMAN_PATH "/service/wifi_"

#define WIFI_ESSID_LEN						128
#define WIFI_MAX_WPSPIN_LEN					8
#define WIFI_BSSID_LEN						17
#define WIFI_MAX_PSK_PASSPHRASE_LEN			65
#define WIFI_MAX_WEP_KEY_LEN				26

#define HOMESCREEN_SERVICE				"org.agl.homescreen"
#define HOMESCREEN_ICON_INTERFACE		"org.agl.statusbar"
#define HOMESCREEN_ICON_PATH			"/StatusBar"
#define HOMESCREEN_WIFI_ICON_POSITION		0


#define AGENT_PATH "/net/connman/Agent"
#define AGENT_SERVICE "org.agent"

#define DBUS_REPLY_TIMEOUT (120 * 1000)
#define DBUS_REPLY_TIMEOUT_SHORT (10 * 1000)


struct gdbus_connection_data_s{
	GDBusConnection *connection;
	int conn_ref_count;
	GCancellable *cancellable;
	void *handle_libnetwork;
};


struct wifiStatus {
	unsigned int state;
	unsigned int connected;
};

struct security_profile{
	char *sec_type;
	char *enc_type;
	char *wepKey;
	char *pskKey;
	unsigned int PassphraseRequired;
	unsigned int wps_support;
};

struct wifi_net{
	char *method;
	char *IPaddress;
	char *netmask;
};

struct wifi_profile_info{
	char *ESSID;
	char *NetworkPath;
	char *state;
	unsigned int Strength;
	struct security_profile Security;
	struct wifi_net wifiNetwork;
};

enum wifiStates {BAR_NO, BAR_1, BAR_2, BAR_3, BAR_FULL};

typedef void(*callback)(int number, const char* asciidata);
void register_callbackSecurity(callback ptr);
void register_callbackWiFiList(callback ptr);



int extract_values(GVariantIter *content, struct wifi_profile_info* wifiProfile);
int wifi_state(struct wifiStatus *status);
GError* do_wifiActivate();
GError*  do_wifiDeactivate();
GError* do_wifiScan();
GError* do_displayScan(GSList **wifi_list);
GError* do_connectNetwork(gchar *object);
GError* do_disconnectNetwork(gchar *object);

GError* create_agent(GDBusConnection *connection);
GError* stop_agent(GDBusConnection *connection);

GError* setHMIStatus(enum wifiStates);

void registerPasskey(gchar *object);
GError* sendPasskey(gchar *object);

GError* do_initialize();
GError* do_initialize();





