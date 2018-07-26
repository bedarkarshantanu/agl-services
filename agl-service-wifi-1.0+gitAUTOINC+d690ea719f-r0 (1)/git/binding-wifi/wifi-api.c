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

/**
 * \file
 *
 * \brief Implementation of WiFi Binder for AGL's App Framework
 *
 * \author ALPS Electric
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <json-c/json.h>

#define AFB_BINDING_VERSION 2
#include <afb/afb-binding.h>

#include "wifi-api.h"
#include "wifi-connman.h"



static int need_passkey_flag = 0;
static int passkey_not_correct_flag = 0;

char *passkey;
callback ptr_my_callback;
callback wifiListChanged_clbck;

GSList *wifi_list = NULL;


/**
 * \brief Input the passkey for connection to AP.
 *
 * \param[in] passkey pasword for the network
 *
 * The user should first subscribe for the event 'passkey' and then provide passkey
 * when this event has been received.
 *
 * */
static void insertpasskey(struct afb_req request) {


    const char *passkey_from_user;

    /* retrieves the argument, expects passkey string */
    passkey_from_user = afb_req_value(request, "passkey");

    AFB_NOTICE("Passkey inserted: %s\n", passkey_from_user);

    sendPasskey(passkey_from_user);


    if (passkey != NULL) {

        registerPasskey(passkey);
    } else {
        AFB_NOTICE("Please enter the passkey first\n");

    }
}



struct event
{
    struct event *next;
    struct afb_event event;
    char name[];
};

static struct event *events = NULL;

/**
 * \internal
 * \brief Helper function that searches for a specific event.
 *
 * \param[in] name of the event
 */
static struct event *event_get(const char *name)
{
    struct event *e = events;
    while(e && strcmp(e->name, name))
        e = e->next;
    return e;
}

/**
 * \internal
 * \brief Helper function that actually pushes the event.
 *
 * \param[in] name of the event
 * \param[in] *args json object that contains data for the event
 *
 */
static int do_event_push(struct json_object *args, const char *name)
{
    struct event *e;
    e = event_get(name);
    return e ? afb_event_push(e->event, json_object_get(args)) : -1;
}

/**
 *
 * \brief Notify user that passkey is necessary.
 *
 * \param[in] number additional integer data produced by Agent
 * \param[in] asciidata additional ascii data produced by Agent
 *
 * This function is called from the registered agent on RequestInput() call.
 *
 */
void ask_for_passkey(int number, const char* asciidata) {
    AFB_NOTICE("Passkey for %s network needed.", asciidata);
    AFB_NOTICE("Sending event.");

    json_object *jresp = json_object_new_object();

    json_object *int1 = json_object_new_int(number);
    json_object *string = json_object_new_string(asciidata);

    json_object_object_add(jresp, "data1", int1);
    json_object_object_add(jresp, "data2", string);


    do_event_push(jresp, "passkey");
}

/**
 * \internal
 * \brief Notify GUI that wifi list has changed.
 *
 * \param[in] number number of event that caused the callback
 * \param[in] asciidata additional data, e.g "BSSRemoved"
 *
 * User should first subscribe for the event 'networkList' and then wait for this event.
 * When notification comes, update the list if networks by scan_result call.
 *
 */
void wifiListChanged(int number, const char* asciidata) {

    //WARNING(afbitf, "wifiListChanged, reason:%d, %s",number, asciidata );


    json_object *jresp = json_object_new_object();

    json_object *int1 = json_object_new_int(number);
    json_object *string = json_object_new_string(asciidata);

    json_object_object_add(jresp, "data1", int1);
    json_object_object_add(jresp, "data2", string);

    do_event_push(jresp, "networkList");



}


/**
 * \brief Initializes the binder and activates the WiFi HW, should be called first.
 *
 * \param[in] request no specific data needed
 *
 * \return result of the request, either "success" or "failed" with description of error
 *
 * This will fail if
 * 	- agent for handling passkey requests cannot be registered
 * 	- some error is returned from connman
 */
static void activate(struct afb_req request) /*AFB_SESSION_CHECK*/
{
	json_object *jresp;
	GError *error = NULL;

	if (ptr_my_callback == NULL) {

        ptr_my_callback = ask_for_passkey;
        register_callbackSecurity(ptr_my_callback);

	}

    if (wifiListChanged_clbck == NULL) {

        wifiListChanged_clbck = wifiListChanged;
        register_callbackWiFiList(wifiListChanged_clbck);

    }

    jresp = json_object_new_object();
    json_object_object_add(jresp, "activation", json_object_new_string("on"));

    error = do_wifiActivate();

    if (error == NULL) {

        afb_req_success(request, jresp, "Wi-Fi - Activated");

    } else

        afb_req_fail(request, "failed", error->message);

}

/**
 * \brief Deinitializes the binder and deactivates the WiFi HW.
 *
 * \param[in] request no specific data needed
 *
 * \return result of the request, either "success" or "failed" with description of error
 *
 * This will fail if
 * 	- agent for handling passkey requests cannot be unregistered
 * 	- some error is returned from connman
 *
 */
static void deactivate(struct afb_req request) /*AFB_SESSION_CHECK*/
{

	json_object *jresp;
	GError *error = NULL;

	ptr_my_callback = NULL;

	jresp = json_object_new_object();
	json_object_object_add(jresp, "deactivation", json_object_new_string("on"));

	error = do_wifiDeactivate();

	if (error == NULL) {
		setHMIStatus(BAR_NO);
		afb_req_success(request, jresp, "Wi-Fi - Activated");

	} else

		afb_req_fail(request, "failed", error->message);
}

/**
 * \brief Starts scan for access points.
 *
 * \param[in] request no specific data needed
 *
 * \return result of the request, either "success" or "failed" with description of error
 *
 * User should first subscribe for the event 'networkList' and then wait for event to come.
 * When notification comes, update the list of networks by scan_result call.
 */
static void scan(struct afb_req request) /*AFB_SESSION_NONE*/
{
	GError *error = NULL;

	error = do_wifiScan();

	if (error == NULL) {

		afb_req_success(request, NULL, "Wi-Fi - Scan success");

	} else

		afb_req_fail(request, "failed", error->message);

}

/**
 * \brief Return network list.
 *
 * \param[in] request no specific data needed
 *
 * \return result of the request, either "success" or "failed" with description of error \n
 * 		   result is array of following json objects: Number, Strength, ESSID, Security, IPAddress, State.
 * 		   E.g. {"Number":0,"Strength":82,"ESSID":"wifidata02","Security":"ieee8021x","IPAddress":"unsigned","State":"idle"}, or \n
 * 		   {"Number":1,"Strength":51,"ESSID":"ALCZM","Security":"WPA-PSK","IPAddress":"192.168.1.124","State":"ready"}
 */
static void scan_result(struct afb_req request) /*AFB_SESSION_CHECK*/
{
	struct wifi_profile_info *wifiProfile = NULL;
	GSList *list = NULL;
	GSList *holdMe = NULL;
	wifi_list = NULL;
	char *essid = NULL;
	char *address = NULL;
	char *security = NULL;
	char *state = NULL;
    unsigned int strength = 0;
	int number = 0;
	GError *error = NULL;

	error = do_displayScan(&wifi_list); /*Get wifi scan list*/
	if (error == NULL) {
		json_object *my_array = json_object_new_array();

		for (list = wifi_list; list; list = list->next) { /*extract wifi scan result*/
			wifiProfile = (struct wifi_profile_info *) list->data;
			security = wifiProfile->Security.sec_type;
			strength = wifiProfile->Strength;
			//if (essid == NULL || security == NULL)
			//	continue;

			essid = wifiProfile->ESSID == NULL ?
					"HiddenSSID" : wifiProfile->ESSID;
			address =
					wifiProfile->wifiNetwork.IPaddress == NULL ?
							"unsigned" : wifiProfile->wifiNetwork.IPaddress;
			state = wifiProfile->state;
			//TODO: is there a case when security is NULL?

			json_object *int1 = json_object_new_int(number);
			json_object *int2 = json_object_new_int(strength);
			json_object *jstring1 = json_object_new_string(essid);
			json_object *jstring2 = json_object_new_string(security);
			json_object *jstring3 = json_object_new_string(address);
			json_object *jstring4 = json_object_new_string(state);

			json_object *jresp = json_object_new_object();
			json_object_object_add(jresp, "Number", int1);
			json_object_object_add(jresp, "Strength", int2);
			json_object_object_add(jresp, "ESSID", jstring1);
			json_object_object_add(jresp, "Security", jstring2);
			json_object_object_add(jresp, "IPAddress", jstring3);
			json_object_object_add(jresp, "State", jstring4);

            AFB_DEBUG("The object json: %s\n", json_object_to_json_string(jresp));

			/*input each scan result into my_array*/
			json_object_array_add(my_array, jresp);
			number += 1;

                        //set the HMI icon according to strength, only if "ready" or "online"

                        int error = 0;
                        struct wifiStatus *status;

                        status = g_try_malloc0(sizeof(struct wifiStatus));
                        error = wifi_state(status); /*get current status of power and connection*/
                        if (!error) {

                            if (status->connected == 1) {

                               if ((strcmp(state, "ready") == 0)
                                    || ((strcmp(state, "online") == 0))) {

                                    if (strength < 30)
                                        setHMIStatus(BAR_1);
                                    else if (strength < 50)
                                        setHMIStatus(BAR_2);
                                    else if (strength < 70)
                                        setHMIStatus(BAR_3);
                                    else
                                        setHMIStatus(BAR_FULL);
                               }
                            } else
                              setHMIStatus(BAR_NO);
                      }

		}
		while (list != NULL) {
			holdMe = list->next;
			g_free(list);
			list = holdMe;
		}
		afb_req_success(request, my_array, "Wi-Fi - Scan Result is Displayed");
	} else
		afb_req_fail(request, "failed", error->message);
}

/**
 * \brief Connects to a specific network.
 *
 * \param[in] request Number of network to connect to.
 *
 * \return result of the request, either "success" or "failed" with description of error
 *
 * Specify number of network to connect to obtained by scan_result().
 * User should first subscribe for the event 'passkey', if passkey
 * is needed for connection this event is pushed.
 *
 */
static void connect(struct afb_req request) {

	struct wifi_profile_info *wifiProfileToConnect = NULL;

	const char *network;
	int network_index = 0;
	GError *error = NULL;
	GSList *item = NULL;

	/* retrieves the argument, expects the network number */
	network = afb_req_value(request, "network");

	if (network == NULL)
		//TODO:better error message
		afb_req_fail(request, "failed",
				"specify a network number to connect to");

	else {
		network_index = atoi(network);
        AFB_NOTICE("Joining network number %d\n", network_index);


	}

	//get information about desired network
	item = g_slist_nth_data(wifi_list, network_index);

	if (item == NULL) {
		//Index starts from 1
        AFB_ERROR("Network with number %d not found.\n", network_index + 1);
		//TODO:better error message
		afb_req_fail(request, "failed", "bad arguments");
	}

	else {
		wifiProfileToConnect = (struct wifi_profile_info *) item;
        AFB_INFO("Name: %s, strength: %d, %s\n", wifiProfileToConnect->ESSID,
				wifiProfileToConnect->Strength,
				wifiProfileToConnect->NetworkPath);
	}
	error = do_connectNetwork(wifiProfileToConnect->NetworkPath);

	if (error == NULL)
		afb_req_success(request, NULL, NULL);

	else if (passkey_not_correct_flag) {
		need_passkey_flag = 0;
		passkey_not_correct_flag = 0;
		afb_req_fail(request, "passkey-incorrect", NULL);
	} else if (need_passkey_flag) {
		need_passkey_flag = 0;
		afb_req_fail(request, "need-passkey", NULL);

	} else
		afb_req_fail(request, "failed", error->message);
}

/**
 * \brief Disconnects from a network.
 *
 * \param[in] request number of network to disconnect from
 *
 * \return result of the request, either "success" or "failed" with description of error
 *
 * Specify number of network to disconnect from obtained by scan_result().
 */
static void disconnect(struct afb_req request) {

	struct wifi_profile_info *wifiProfileToConnect = NULL;

	const char *network;
	int network_index = 0;
	GError *error = NULL;
	GSList *item = NULL;

	/* retrieves the argument, expects the network number */
	network = afb_req_value(request, "network");

	if (network == NULL)
		//TODO:better error message
		afb_req_fail(request, "failed",
				"specify a network number to disconnect from");

	else {
		network_index = atoi(network);
        AFB_NOTICE("Joining network number %d\n", network_index);

	}

	//get information about desired network
	item = g_slist_nth_data(wifi_list, network_index);

	if (item == NULL) {
		//Index starts from 1
        AFB_ERROR("Network with number %d not found.\n", network_index + 1);
		//TODO:better error message
		afb_req_fail(request, "failed", "bad arguments");
	}

	else {
		wifiProfileToConnect = (struct wifi_profile_info *) item;
        AFB_INFO("Name: %s, strength: %d, %s\n", wifiProfileToConnect->ESSID,
				wifiProfileToConnect->Strength,
				wifiProfileToConnect->NetworkPath);
	}
	error = do_disconnectNetwork(wifiProfileToConnect->NetworkPath);

	if (error == NULL)
		afb_req_success(request, NULL, NULL);
	else
		afb_req_fail(request, "failed", error->message);
}

/**
 * \brief Return current status of a connection.
 *
 * \param[in] request no specific data needed
 *
 * \return result of the request, either "success" with status or "failed" with description of error
 */
static void status(struct afb_req request) {
	int error = 0;
	wifi_list = NULL;
	struct wifiStatus *status;
	json_object *jresp = json_object_new_object();

	status = g_try_malloc0(sizeof(struct wifiStatus));
	error = wifi_state(status); /*get current status of power and connection*/
	if (!error) {
		if (status->state == 0) {/*Wi-Fi is OFF*/
			json_object_object_add(jresp, "Power",
					json_object_new_string("OFF"));
			//json_object_object_add(jresp, "Connection", json_object_new_string("Disconnected"));
            AFB_DEBUG("Wi-Fi OFF\n");
		} else {/*Wi-Fi is ON*/
			json_object_object_add(jresp, "Power",
					json_object_new_string("ON"));
			if (status->connected == 0) {/*disconnected*/
				json_object_object_add(jresp, "Connection",
						json_object_new_string("Disconnected"));
                AFB_DEBUG("Wi-Fi ON - Disconnected \n");
			} else {/*Connected*/
				json_object_object_add(jresp, "Connection",
						json_object_new_string("Connected"));
                AFB_DEBUG("Wi-Fi ON - Connected \n");
			}
		}
		afb_req_success(request, jresp, "Wi-Fi - Connection Status Checked");
	} else {
		afb_req_fail(request, "failed", "Wi-Fi - Connection Status unknown");
	}
}

/**
 * \internal
 * \brief Helper functions that actually creates event of the name.
 *
 * \param[in] name to add
 *
 * \return result of the request, either "success" or "failed" with description of error
 */
static int event_add(const char *name)
{
    struct event *e;

    /* check valid name */
    e = event_get(name);
    if (e) return -1;

    /* creation */
    e = malloc(strlen(name) + sizeof *e + 1);
    if (!e) return -1;
    strcpy(e->name, name);

    /* make the event */
    e->event = afb_daemon_make_event(name);
    if (!e->event.closure) { free(e); return -1; }

    /* link */
    e->next = events;
    events = e;
    return 0;
}

/**
 * \internal
 * \brief Helper functions to subscribe for the event of name.
 *
 * \param[in] request name to subscribe for
 *
 * \return result of the request, either "success" or "failed" with description of error
 */
static int event_subscribe(struct afb_req request, const char *name)
{
    struct event *e;
    e = event_get(name);
    return e ? afb_req_subscribe(request, e->event) : -1;
}

/**
 * \internal
 * \brief Helper functions to unsubscribe for the event of name.
 *
 * \param[in] request name to unsubscribe for
 *
 * \return result of the request, either "success" or "failed" with description of error
 */
static int event_unsubscribe(struct afb_req request, const char *name)
{
    struct event *e;
    e = event_get(name);
    return e ? afb_req_unsubscribe(request, e->event) : -1;
}


/**
 * \brief Subscribes for the event of name.
 *
 * \param[in] request name to subscribe for
 *
 * \return result of the request, either "success" or "failed" with description of error
 */
static void subscribe(struct afb_req request)
{
    const char *name = afb_req_value(request, "value");

    if (name == NULL)
        afb_req_fail(request, "failed", "bad arguments");
    else if (0 != event_subscribe(request, name))
        afb_req_fail(request, "failed", "subscription error");
    else
        afb_req_success(request, NULL, NULL);
}


/**
 * \brief Unsubscribes for the event of name.
 *
 * \param[in] request name to unsubscribe for
 *
 * \return result of the request, either "success" or "failed" with description of error
 */
static void unsubscribe(struct afb_req request)
{
    const char *name = afb_req_value(request, "value");

    if (name == NULL)
        afb_req_fail(request, "failed", "bad arguments");
    else if (0 != event_unsubscribe(request, name))
        afb_req_fail(request, "failed", "unsubscription error");
    else
        afb_req_success(request, NULL, NULL);
}


/*
 * array of the verbs exported to afb-daemon
 */
static const struct afb_verb_v2 binding_verbs[] = {
/* VERB'S NAME             FUNCTION TO CALL            SHORT DESCRIPTION */
{ .verb = "activate",      .callback = activate,       .info = "Activate Wi-Fi" },
{ .verb = "deactivate",    .callback = deactivate,     .info ="Deactivate Wi-Fi" },
{ .verb = "scan",          .callback = scan,           .info ="Scanning Wi-Fi" },
{ .verb = "scan_result",   .callback = scan_result,    .info = "Get scan result Wi-Fi" },
{ .verb = "connect",       .callback = connect,        .info ="Connecting to Access Point" },
{ .verb = "status",        .callback = status,         .info ="Check connection status" },
{ .verb = "disconnect",    .callback = disconnect,     .info ="Disconnecting connection" },
{ .verb = "insertpasskey", .callback = insertpasskey,  .info ="inputs the passkey after it has been requsted"},
{ .verb = "subscribe",     .callback = subscribe,      .info ="unsubscribes to the event of 'name'"},
{ .verb = "unsubscribe",   .callback = unsubscribe,    .info ="unsubscribes to the event of 'name'"},

{ } /* marker for end of the array */
};


gboolean initial_homescreen(gpointer ptr)
{
    struct wifiStatus *status = g_try_malloc0(sizeof(struct wifiStatus));
    GError *error = NULL;
    int ret = 0;

    ret = wifi_state(status);

    if (ret < 0) {
        g_object_unref(status);
        return FALSE;
    }

    error = setHMIStatus(status->connected == 1 ? BAR_FULL : BAR_NO);
    g_error_free(error);

    return error != NULL;
}

static void *thread_func()
{
    g_timeout_add_seconds(5, initial_homescreen, NULL);
    g_main_loop_run(g_main_loop_new(NULL, FALSE));
}

static int init()
{
    pthread_t thread_id;

    event_add("passkey");
    event_add("networkList");

    return pthread_create(&thread_id, NULL, thread_func, NULL);
}

/*
 * description of the binding for afb-daemon
 */
const struct afb_binding_v2 afbBindingV2 = {
    .api = "wifi-manager", /* the API name (or binding name or prefix) */
    .specification = "wifi API", /* short description of of the binding */
    .verbs = binding_verbs, /* the array describing the verbs of the API */
    .init = init,
};
