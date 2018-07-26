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
#include <errno.h>

#include <gio/gio.h>
#include "wifi-connman.h"

static GMainLoop *loop = NULL;

static GDBusNodeInfo *introspection_data = NULL;

GDBusConnection *connectionAgent;

GDBusMethodInvocation *invocation_passkey = NULL;

/* Introspection data for the agent service */
static const gchar introspection_xml[] = "<node>"
		"  <interface name='net.connman.Agent'>"
		"    <method name='RequestInput'>"
		"	   <arg type='o' name='service' direction='in'/>"
		"	   <arg type='a{sv}' name='fields' direction='in'/>"
		"	   <arg type='a{sv}' name='fields' direction='out'/>"
		"    </method>"
		"    <method name='ReportError'>"
		"	   <arg type='o' name='service' direction='in'/>"
		"	   <arg type='s' name='error' direction='in'/>"
		"    </method>"
		"  </interface>"
		"</node>";

callback password_callback;
callback wifiListChanged_callback;

static void handle_method_call(GDBusConnection *connection, const gchar *sender,
		const gchar *object_path, const gchar *interface_name,
		const gchar *method_name, GVariant *parameters,
		GDBusMethodInvocation *invocation, gpointer user_data) {
	//MyObject *myobj = user_data;

	if (g_strcmp0(method_name, "RequestInput") == 0) {
        AFB_DEBUG("RequestInput method on Agent interface has been called\n");

		invocation_passkey = invocation;

		GVariantIter *array;
		gchar * object_path;
		g_variant_get(parameters, "(oa{sv})", &object_path, &array);
		//TODO: get only object path for now, complete parameters are

		/*
		 object path "/net/connman/service/wifi_d85d4c880b1a_4c656e6f766f204b3520506c7573_managed_psk"
		 array [
		 dict entry(
		 string "Passphrase"
		 variant             array [
		 dict entry(
		 string "Type"
		 variant                      string "psk"
		 )
		 dict entry(
		 string "Requirement"
		 variant                      string "mandatory"
		 )
		 ]
		 )
		 ]
		 */
        AFB_NOTICE("Passphrase requested for network : %s\n", object_path);
        (*password_callback)(0, object_path);


	}

	if (g_strcmp0(method_name, "ReportError") == 0) {
        AFB_ERROR("ReportError method on Agent interface has een called\n");

		gchar *error_string; // = NULL;

		gchar * object_path;
		g_variant_get(parameters, "(os)", &object_path, &error_string);

        AFB_ERROR("Error %s for %s\n", error_string, object_path);

		if (g_strcmp0(error_string, "invalid-key") == 0) {

            AFB_WARNING( "Passkey is not correct.\n");
            (*password_callback)(1, "invalid-key");

		}

	}
}

GError* sendPasskey(gchar *passkey) {

	GVariantBuilder *builder;
	GVariant *value = NULL;

    AFB_NOTICE("Passkey to send: %s\n", passkey);

	builder = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));

	g_variant_builder_add(builder, "{sv}", "Passphrase",
			g_variant_new_string(passkey));

	value = g_variant_new("(a{sv})", builder);

	g_dbus_method_invocation_return_value(invocation_passkey, value);

	return NULL;

}

static const GDBusInterfaceVTable interface_vtable = { handle_method_call, NULL,
		NULL };

static void on_bus_acquired(GDBusConnection *connection, const gchar *name,
		gpointer user_data) {
	//MyObject *myobj = user_data;
	guint registration_id;

	registration_id = g_dbus_connection_register_object(connection,
			"/net/connman/Agent", introspection_data->interfaces[0],
			&interface_vtable, NULL, NULL, /* user_data_free_func */
			NULL); /* GError** */
	//TODO: make some proper error message rather than exiting
	//g_assert(registration_id > 0);

	return NULL;
}

static void test_signal_handler (GDBusConnection *connection, const gchar *sender_name, const gchar *object_path, const gchar *interface_name, const gchar *signal_name, GVariant *parameters, gpointer user_data) {

    //do not parse, just check what has changed and make callback -
    // we need to refresh completelist anyway..
    if (g_strcmp0(signal_name, "PropertiesChanged") == 0) {

        (*wifiListChanged_callback)(1, "PropertiesChanged");
    }
    else if (g_strcmp0(signal_name, "BSSRemoved") == 0) {
        (*wifiListChanged_callback)(2, "BSSRemoved");
        }

    else if (g_strcmp0(signal_name, "BSSAdded") == 0) {
            (*wifiListChanged_callback)(2, "BSSAdded");
            }
    else AFB_WARNING("unhandled signal %s %s %s, %s", sender_name, object_path, interface_name, signal_name);


}

void* register_agent(void *data) {

	guint owner_id;

    guint networkChangedID;

	introspection_data = g_dbus_node_info_new_for_xml(introspection_xml, NULL);
	g_assert(introspection_data != NULL);

	//myobj = g_object_new(my_object_get_type(), NULL);

//	owner_id = g_bus_own_name(G_BUS_TYPE_SYSTEM, "org.agent",
//			G_BUS_NAME_OWNER_FLAGS_NONE, on_bus_acquired, on_name_acquired,
//			on_name_lost, myobj,
//			NULL);

//FIXME: ALLOW_REPLACEMENT for now, make proper deinitialization
	owner_id = g_bus_own_name(G_BUS_TYPE_SYSTEM, AGENT_SERVICE,
			G_BUS_NAME_OWNER_FLAGS_REPLACE, on_bus_acquired, NULL, NULL, NULL,
			NULL);
	//G_BUS_NAME_OWNER_FLAGS_NONE G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT


    //"net.connman.Manager", "ServicesChanged",
    networkChangedID = g_dbus_connection_signal_subscribe(connectionAgent, NULL, "fi.w1.wpa_supplicant1.Interface", NULL, NULL, NULL, G_DBUS_SIGNAL_FLAGS_NONE, test_signal_handler, NULL, NULL);

    g_assert(networkChangedID !=0);

	loop = g_main_loop_new(NULL, FALSE);

	//sleep(10);
	g_main_loop_run(loop);

	g_bus_unown_name(owner_id);

	g_dbus_node_info_unref(introspection_data);

	//g_object_unref(myobj);

	return NULL;

}

GError* create_agent(GDBusConnection *connection) {

	int err = -1;
	pthread_t tid[1];


    connectionAgent = connection;

	err = pthread_create((&tid[0]), NULL, register_agent, NULL);

	if (err != 0) {
        AFB_ERROR("\ncan't create thread :[%d]", err);
        AFB_ERROR("Fatal error!\n\n");
		return NULL;
	}

	GVariant *message = NULL;
	GError *error = NULL;

	GVariant *params = NULL;

	char *agent_path = AGENT_PATH;

	params = g_variant_new("(o)", agent_path);

	message = g_dbus_connection_call_sync(connection, CONNMAN_SERVICE,
	CONNMAN_MANAGER_PATH, CONNMAN_MANAGER_INTERFACE, "RegisterAgent", params,
			NULL, G_DBUS_CALL_FLAGS_NONE,
			DBUS_REPLY_TIMEOUT, NULL, &error);

	if (error) {
        AFB_ERROR("error: %d:%s\n", error->code, error->message);
		
		return error;

	} else {
        AFB_INFO("Agent registered\n");
		return NULL;
	}

}

GError* stop_agent(GDBusConnection *connection) {

	GVariant *message = NULL;
	GError *error = NULL;


	GVariant *params = NULL;

	char *agent_path = AGENT_PATH;


	params = g_variant_new("(o)", agent_path);

	message = g_dbus_connection_call_sync(connection, CONNMAN_SERVICE,
	CONNMAN_MANAGER_PATH, CONNMAN_MANAGER_INTERFACE, "UnregisterAgent", params,
			NULL, G_DBUS_CALL_FLAGS_NONE,
			DBUS_REPLY_TIMEOUT, NULL, &error);

	if (error) {
        AFB_ERROR("error: %d:%s\n", error->code, error->message);
		return error;

	} else {
        AFB_DEBUG("Agent unregistered\n");
		return NULL;
	}

}

void register_callbackSecurity(callback callback_function) {

	password_callback = callback_function;

}

void register_callbackWiFiList(callback callback_function) {

    wifiListChanged_callback = callback_function;

}


