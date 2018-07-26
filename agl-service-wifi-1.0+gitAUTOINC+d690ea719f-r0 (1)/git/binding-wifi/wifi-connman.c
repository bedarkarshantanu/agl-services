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
#include <glib.h>
//#include <dbus/dbus.h>
#include <gio/gio.h>
#include <glib-object.h>

#include "wifi-api.h"
#include "wifi-connman.h"

static __thread struct security_profile Security = { NULL, NULL, NULL, NULL, 0,
		0 };

int extract_values(GVariantIter *content, struct wifi_profile_info* wifiProfile) {
	GVariant *var = NULL;
	GVariant *subvar = NULL;
	GVariantIter *array;
	const gchar *key = NULL;
	const gchar *subkey = NULL;
	const gchar *value_char = NULL;
	GVariantIter *content_sub;
    unsigned int value_int;
	gsize length;

	while (g_variant_iter_loop(content, "{sv}", &key, &var)) {
		if (g_strcmp0(key, "Name") == 0) {
			value_char = g_variant_get_string(var, &length);
			wifiProfile->ESSID = (char *) value_char;
		} else if (g_strcmp0(key, "Security") == 0) {
			g_variant_get(var, "as", &content_sub);
			while (g_variant_iter_loop(content_sub, "s", &value_char)) {
				if (g_strcmp0(value_char, "none") == 0)
					wifiProfile->Security.sec_type = "Open";
				else if (g_strcmp0(value_char, "wep") == 0)
					wifiProfile->Security.sec_type = "WEP";
				else if (g_strcmp0(value_char, "psk") == 0)
					wifiProfile->Security.sec_type = "WPA-PSK";
				else if (g_strcmp0(value_char, "ieee8021x") == 0)
					wifiProfile->Security.sec_type = "ieee8021x";
				else if (g_strcmp0(value_char, "wpa") == 0)
					wifiProfile->Security.sec_type = "WPA-PSK";
				else if (g_strcmp0(value_char, "rsn") == 0)
					wifiProfile->Security.sec_type = "WPA2-PSK";
				else if (g_strcmp0(value_char, "wps") == 0)
					wifiProfile->Security.wps_support = 1;
				else
					Security.sec_type = "Open";
			}
		} else if (g_strcmp0(key, "Strength") == 0) {
			value_int = (unsigned int) g_variant_get_byte(var);
			wifiProfile->Strength = value_int;
		} else if (g_strcmp0(key, "State") == 0) {
			value_char = g_variant_get_string(var, &length);
			wifiProfile->state = (char *) value_char;
		} else if (g_strcmp0(key, "IPv4") == 0) {
			g_variant_get(var, "a{sv}", &array);
			while (g_variant_iter_loop(array, "{sv}", &subkey, &subvar)) {
				if (g_strcmp0(subkey, "Method") == 0) {
					value_char = g_variant_get_string(subvar, &length);
					if (g_strcmp0(value_char, "dhcp") == 0)
						wifiProfile->wifiNetwork.method = "dhcp";
					else if (g_strcmp0(value_char, "manual") == 0)
						wifiProfile->wifiNetwork.method = "manual";
					else if (g_strcmp0(value_char, "fixed") == 0)
						wifiProfile->wifiNetwork.method = "fix";
					else if (g_strcmp0(value_char, "off") == 0)
						wifiProfile->wifiNetwork.method = "off";
				} else if (g_strcmp0(subkey, "Address") == 0) {
					value_char = g_variant_get_string(subvar, &length);
					wifiProfile->wifiNetwork.IPaddress = (char *) value_char;
				} else if (g_strcmp0(subkey, "Netmask") == 0) {
					value_char = g_variant_get_string(subvar, &length);
					wifiProfile->wifiNetwork.netmask = (char *) value_char;
				}
			}
		}
	}
	return 0;
}

int wifi_state(struct wifiStatus *status) {
	GError *error = NULL;
	GVariant *message = NULL;
	GVariantIter *array;
	GDBusConnection *connection;
	GVariant *var = NULL;
	const gchar *key = NULL;
	gboolean value_bool;

	connection = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
	if (connection == NULL) {
        AFB_ERROR("Cannot connect to D-Bus, %s", error->message);
		return -1;
	}
	message = g_dbus_connection_call_sync(connection, CONNMAN_SERVICE,
	CONNMAN_TECHNOLOGY_PATH, CONNMAN_TECHNOLOGY_INTERFACE, "GetProperties",
			NULL, NULL, G_DBUS_CALL_FLAGS_NONE,
			DBUS_REPLY_TIMEOUT, NULL, &error);
	if (message == NULL) {
        AFB_ERROR("Error %s while calling GetProperties method", error->message);
        return -1;
	}
	g_variant_get(message, "(a{sv})", &array);
	while (g_variant_iter_loop(array, "{sv}", &key, &var)) {
		if (g_strcmp0(key, "Powered") == 0) {
			value_bool = g_variant_get_boolean(var);
			if (value_bool)
				status->state = 1;
			else
				status->state = 0;
		} else if (g_strcmp0(key, "Connected") == 0) {
			value_bool = g_variant_get_boolean(var);
			if (value_bool)
				status->connected = 1;
			else
				status->connected = 0;
		}
	}
	g_variant_iter_free(array);
	g_variant_unref(message);

	return 0;
}

GError* do_wifiActivate() {
	GVariant *params = NULL;
	params = g_variant_new("(sv)", "Powered", g_variant_new_boolean(TRUE));
	GDBusConnection *connection;
	GError *error = NULL;

	connection = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);

	if (connection == NULL) {
        AFB_ERROR("Cannot connect to D-Bus, %s", error->message);
        return error;
	}

	//create the agent to handle security
	error = create_agent(connection);

	if (error)
		//This is fatal error, without agent secured networks can not be handled
		return error;

	g_dbus_connection_call(connection, CONNMAN_SERVICE,
	CONNMAN_WIFI_TECHNOLOGY_PREFIX, CONNMAN_TECHNOLOGY_INTERFACE, "SetProperty",
			params, NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL, &error);

	if (error) {
        AFB_ERROR("Error %s while calling SetProperty method", error->message);

		return error;
	}

	 else {
        AFB_NOTICE("Power ON succeeded\n");
		return NULL;
	}

}

GError*  do_wifiDeactivate() {
	GVariant *params = NULL;
	params = g_variant_new("(sv)", "Powered", g_variant_new_boolean(FALSE));
	GDBusConnection *connection;
	GError *error = NULL;

	/*connection = gdbus_conn->connection;*/
	connection = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
	if (connection == NULL) {
        AFB_ERROR("Cannot connect to D-Bus, %s", error->message);
        return error;
	}

	//create the agent to handle security
	error = stop_agent(connection);

	if (error) {
        AFB_WARNING( "Error while unregistering the agent, ignoring.");

	}

	g_dbus_connection_call(connection, CONNMAN_SERVICE,
	CONNMAN_WIFI_TECHNOLOGY_PREFIX, CONNMAN_TECHNOLOGY_INTERFACE, "SetProperty",
			params, NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL, &error);

	if (error) {
        AFB_ERROR("Error %s while calling SetProperty method", error->message);

		return error;
	}

	else {
        AFB_NOTICE( "Power OFF succeeded\n");
		return NULL;
	}
}

GError* do_wifiScan() {
	GDBusConnection *connection;
	GError *error = NULL;

	/*connection = gdbus_conn->connection;*/
	connection = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
	if (connection == NULL) {
        AFB_ERROR("Cannot connect to D-Bus, %s", error->message);
        return error;
	}

	g_dbus_connection_call(connection, CONNMAN_SERVICE,
	CONNMAN_WIFI_TECHNOLOGY_PREFIX, CONNMAN_TECHNOLOGY_INTERFACE, "Scan", NULL,
			NULL, G_DBUS_CALL_FLAGS_NONE, DBUS_REPLY_TIMEOUT, NULL, NULL, &error);
	if (error) {
        AFB_ERROR("Error %s while calling Scan method", error->message);

		return error;
	}

	else {
        AFB_INFO( "Scan succeeded\n");
		return NULL;
	}
}

GError* do_displayScan(GSList **wifi_list) {
	GError *error = NULL;
	GVariant *message = NULL;
	GVariantIter *array;
	gchar *object;
	GVariantIter *content = NULL;
	GDBusConnection *connection;
	struct wifi_profile_info *wifiProfile = NULL;

	/*connection = gdbus_conn->connection;*/
	connection = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
	if (connection == NULL) {
        AFB_ERROR("Cannot connect to D-Bus, %s", error->message);
        return error;
	}
	message = g_dbus_connection_call_sync(connection, CONNMAN_SERVICE,
	CONNMAN_MANAGER_PATH, CONNMAN_MANAGER_INTERFACE, "GetServices", NULL, NULL,
			G_DBUS_CALL_FLAGS_NONE,
			DBUS_REPLY_TIMEOUT, NULL, &error);
	if (message == NULL) {
        AFB_ERROR("Error %s while calling GetServices method", error->message);
        return error;
	}
	g_variant_get(message, "(a(oa{sv}))", &array);
	while (g_variant_iter_loop(array, "(oa{sv})", &object, &content)) {
		if (g_str_has_prefix(object,
		CONNMAN_WIFI_SERVICE_PROFILE_PREFIX) == TRUE) {
			wifiProfile = g_try_malloc0(sizeof(struct wifi_profile_info));

			extract_values(content, wifiProfile);
			wifiProfile->NetworkPath = g_try_malloc0(strlen(object));
			strcpy(wifiProfile->NetworkPath, object);
            AFB_DEBUG(
					"SSID= %s, security= %s, path= %s, Strength= %d, wps support= %d\n",
					wifiProfile->ESSID, wifiProfile->Security.sec_type,
					wifiProfile->NetworkPath, wifiProfile->Strength,
					wifiProfile->Security.wps_support);
            AFB_DEBUG( "method= %s, ip address= %s, netmask= %s\n",
					wifiProfile->wifiNetwork.method,
					wifiProfile->wifiNetwork.IPaddress,
                    wifiProfile->wifiNetwork.netmask);
			*wifi_list = g_slist_append(*wifi_list,
					(struct wifi_profile_info *) wifiProfile);
		}
	}
	g_variant_iter_free(array);

	return NULL;
}

GError* do_connectNetwork(gchar *networkPath) {

    AFB_NOTICE( "Connecting to: %s\n", networkPath);

	GVariant *message = NULL;
	GError *error = NULL;
	GDBusConnection *connection;

	connection = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);

	message = g_dbus_connection_call_sync(connection, CONNMAN_SERVICE,
			networkPath, CONNMAN_SERVICE_INTERFACE, "Connect", NULL, NULL,
			G_DBUS_CALL_FLAGS_NONE,
			DBUS_REPLY_TIMEOUT_SHORT, NULL, &error);

	//TODO: do we need retunrn value in message

	if (error) {
        AFB_ERROR("Error %s while calling Connect method", error->message);
        return error;
	} else {
        AFB_INFO("Connection succeeded\n");
		return NULL;
	}

}

GError* do_disconnectNetwork(gchar *networkPath) {

    AFB_NOTICE( "Connecting to: %s\n", networkPath);

	GVariant *message = NULL;
	GError *error = NULL;
	GDBusConnection *connection;

	connection = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);

    if (connection == NULL) {
        AFB_ERROR("Cannot connect to D-Bus, %s", error->message);
        return error;
    }

	message = g_dbus_connection_call_sync(connection, CONNMAN_SERVICE,
			networkPath, CONNMAN_SERVICE_INTERFACE, "Disconnect", NULL, NULL,
			G_DBUS_CALL_FLAGS_NONE,
			DBUS_REPLY_TIMEOUT, NULL, &error);

	//TODO: do we need return value in message

	if (error) {
        AFB_ERROR("Error %s while calling Disconnect method", error->message);
        return error;
	} else {
        AFB_INFO("Disconnection succeeded\n");
        return NULL;
	}

}

void registerPasskey(gchar *passkey) {

    AFB_INFO("Passkey: %s\n", passkey);
	sendPasskey(passkey);


}

GError* setHMIStatus(enum wifiStates state) {

    gchar *iconString = NULL;
    GDBusConnection *connection;
    GVariant *params = NULL;
    GVariant *message = NULL;
    GError *error = NULL;

    if (state==BAR_NO) iconString = "qrc:/images/Status/HMI_Status_Wifi_NoBars-01.png";
    else if (state==BAR_1) iconString = "qrc:/images/Status/HMI_Status_Wifi_1Bar-01.png";
    else if (state==BAR_2) iconString = "qrc:/images/Status/HMI_Status_Wifi_2Bars-01.png";
    else if (state==BAR_3) iconString = "qrc:/images/Status/HMI_Status_Wifi_3Bars-01.png";
    else if (state==BAR_FULL) iconString = "qrc:/images/Status/HMI_Status_Wifi_Full-01.png";

    connection = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);

    if (connection == NULL) {
        AFB_ERROR("Cannot connect to D-Bus, %s", error->message);
        return error;
    }

    params = g_variant_new("(is)", HOMESCREEN_WIFI_ICON_POSITION, iconString);

    message = g_dbus_connection_call_sync(connection, HOMESCREEN_SERVICE,
    HOMESCREEN_ICON_PATH, HOMESCREEN_ICON_INTERFACE, "setStatusIcon", params,
            NULL, G_DBUS_CALL_FLAGS_NONE,
            DBUS_REPLY_TIMEOUT, NULL, &error);

    if (error) {
        AFB_ERROR("Error %s while setting up the status icon", error->message);

        return error;
    } else {
        return NULL;
    }

}

