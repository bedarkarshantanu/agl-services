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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <json-c/json.h>

#define AFB_BINDING_VERSION 2
#include <afb/afb-binding.h>

#include "bluetooth-manager.h"
#include "bluetooth-agent.h"
#include "bluetooth-api.h"

struct event
{
	struct event *next;
	struct afb_event event;
	char name[];
};

static struct event *events = NULL;

/* searchs the event of name */
static struct event *event_get(const char *name)
{
	struct event *e = events;
	while(e && strcmp(e->name, name))
		e = e->next;
	return e;
}

/* creates the event of name */
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

static int event_subscribe(struct afb_req request, const char *name)
{
	struct event *e;
	e = event_get(name);
	return e ? afb_req_subscribe(request, e->event) : -1;
}

static int event_unsubscribe(struct afb_req request, const char *name)
{
	struct event *e;
	e = event_get(name);
	return e ? afb_req_unsubscribe(request, e->event) : -1;
}

static int event_push(struct json_object *args, const char *name)
{
	struct event *e;
	e = event_get(name);
	return e ? afb_event_push(e->event, json_object_get(args)) : -1;
}

static json_object *new_json_object_parse_avrcp(struct btd_device *BDdevice, unsigned int filter)
{
    json_object *jresp = json_object_new_object();
    json_object *jstring = NULL;

    if (BD_AVRCP_TITLE & filter)
    {
        if (BDdevice->avrcp_title)
        {
            jstring = json_object_new_string(BDdevice->avrcp_title);
        }
        else
        {
            jstring = json_object_new_string("");
        }
        json_object_object_add(jresp, "Title", jstring);
    }

    if (BD_AVRCP_ARTIST & filter)
    {
        if (BDdevice->avrcp_artist)
        {
            jstring = json_object_new_string(BDdevice->avrcp_artist);
        }
        else
        {
            jstring = json_object_new_string("");
        }
        json_object_object_add(jresp, "Artist", jstring);
    }

    if (BD_AVRCP_STATUS & filter)
    {
        if (BDdevice->avrcp_status)
        {
            jstring = json_object_new_string(BDdevice->avrcp_status);
        }
        else
        {
            jstring = json_object_new_string("");
        }
        json_object_object_add(jresp, "Status", jstring);
    }

    if (BD_AVRCP_DURATION & filter)
    {
        json_object_object_add(jresp, "Duration",
                               json_object_new_int(BDdevice->avrcp_duration));
    }

    if (BD_AVRCP_POSITION & filter)
    {
        json_object_object_add(jresp, "Position",
                               json_object_new_int(BDdevice->avrcp_position));
    }

    return jresp;
}

/* create device json object*/
static json_object *new_json_object_from_device(struct btd_device *BDdevice, unsigned int filter)
{
    json_object *jresp = json_object_new_object();
    json_object *jstring = NULL;

    if (BD_PATH & filter)
    {
        if (BDdevice->path)
        {
            jstring = json_object_new_string(BDdevice->path);
        }
        else
        {
            jstring = json_object_new_string("");
        }
        json_object_object_add(jresp, "Path", jstring);
    }

    if (BD_ADDER & filter)
    {
        if (BDdevice->bdaddr)
        {
            jstring = json_object_new_string(BDdevice->bdaddr);
        }
        else
        {
            jstring = json_object_new_string("");
        }
        json_object_object_add(jresp, "Address", jstring);
    }

    if (BD_NAME & filter)
    {
        if (BDdevice->name)
        {
            jstring = json_object_new_string(BDdevice->name);
        }
        else
        {
            jstring = json_object_new_string("");
        }
        json_object_object_add(jresp, "Name", jstring);
    }

    if (BD_PAIRED & filter)
    {
        jstring = (TRUE == BDdevice->paired) ?
            json_object_new_string("True"):json_object_new_string("False");
        json_object_object_add(jresp, "Paired", jstring);
    }

    if (BD_TRUSTED & filter)
    {
        jstring = (TRUE == BDdevice->trusted) ?
            json_object_new_string("True"):json_object_new_string("False");
        json_object_object_add(jresp, "Trusted", jstring);
    }

    if (BD_ACLCONNECTED & filter)
    {
        jstring = (TRUE == BDdevice->connected) ?
            json_object_new_string("True"):json_object_new_string("False");
        json_object_object_add(jresp, "Connected", jstring);
    }

    if (BD_AVCONNECTED & filter)
    {
        jstring = (TRUE == BDdevice->avconnected) ?
            json_object_new_string("True"):json_object_new_string("False");
        json_object_object_add(jresp, "AVPConnected", jstring);

        if (BDdevice->avconnected)
        {
            jstring = new_json_object_parse_avrcp(BDdevice, filter);
            json_object_object_add(jresp, "Metadata", jstring);
        }
    }

    if (BD_TRANSPORT_STATE & filter)
    {
        jstring = BDdevice->transport_state ?
                  json_object_new_string(BDdevice->transport_state) :
                  json_object_new_string("none");
        json_object_object_add(jresp, "TransportState", jstring);
    }

    if (BD_TRANSPORT_VOLUME & filter)
    {
        json_object_object_add(jresp, "TransportVolume",
                               json_object_new_int(BDdevice->transport_volume));
    }

    if (BD_HFPCONNECTED & filter)
    {
        jstring = (TRUE == BDdevice->hfpconnected) ?
            json_object_new_string("True"):json_object_new_string("False");
        json_object_object_add(jresp, "HFPConnected", jstring);
    }

    if (BD_UUID_PROFILES & filter)
    {
        GList *list = BDdevice->uuids;

        if (list)
        {
            json_object *jarray = json_object_new_array();

            for (;list;list=list->next)
            {
                jstring = json_object_new_string(list->data);
                json_object_array_add(jarray, jstring);
            }
            json_object_object_add(jresp, "UUIDs", jarray);
        }
    }

    return jresp;
}

/**/
static void bt_power (struct afb_req request)
{
    LOGD("\n");

    const char *value = afb_req_value (request, "value");
    json_object *jresp = NULL;
    int ret = 0;

    jresp = json_object_new_object();

    /* no "?value=" parameter : return current state */
    if (!value) {
        gboolean power_value;
        ret = adapter_get_powered(&power_value);

        if (0==ret)
        {

            setHMIStatus(ACTIVE);
            (TRUE==power_value)?json_object_object_add (jresp, "power", json_object_new_string ("on"))
                                : json_object_object_add (jresp, "power", json_object_new_string ("off"));
        }
        else
        {
            afb_req_fail (request, "failed", "Unable to get power status");
            return;
        }

    }

    /* "?value=" parameter is "1" or "true" */
    else if ( atoi(value) == 1 || !strcasecmp(value, "true") )
    {
        if (adapter_set_powered (TRUE))
        {
            afb_req_fail (request,"failed","no more radio devices available");
            return;
        }
        json_object_object_add (jresp, "power", json_object_new_string ("on"));
        setHMIStatus(ACTIVE);
    }

    /* "?value=" parameter is "0" or "false" */
    else if ( atoi(value) == 0 || !strcasecmp(value, "false") )
    {
        if (adapter_set_powered (FALSE))
        {
            afb_req_fail (request, "failed", "Unable to release radio device");
            return;
        }

        json_object_object_add (jresp, "power", json_object_new_string("off"));
        setHMIStatus(INACTIVE);
    }
    else
    {
        afb_req_fail (request, "failed", "Invalid value");
        return;
    }

    afb_req_success (request, jresp, "Radio - Power set");
}

/**/
static void bt_start_discovery (struct afb_req request)
{
    LOGD("\n");
    int ret = 0;

    ret = adapter_start_discovery();

    if (ret)
    {
        afb_req_fail (request, "failed", "Unable to start discovery");
        return;
    }

    afb_req_success (request, NULL, NULL);
}

/**/
static void bt_stop_discovery (struct afb_req request)
{
    LOGD("\n");
    int ret = 0;

    ret = adapter_stop_discovery();

    if (ret)
    {
        afb_req_fail (request, "failed", "Unable to stop discovery");
        return;
    }

    afb_req_success (request, NULL, NULL);
}


/**/
static void bt_discovery_result (struct afb_req request)
{
    LOGD("\n");
    GSList *list = NULL;
    GSList *tmp = NULL;
    //adapter_update_devices();
    list = adapter_get_devices_list();
    if (NULL == list)
    {
        afb_req_fail (request, "failed", "No find devices");
        return;
    }

    json_object *jresp = json_object_new_object();
    json_object *my_array = json_object_new_array();

    tmp = list;
    for(;tmp;tmp=tmp->next)
    {
        struct btd_device *BDdevice = tmp->data;
        //LOGD("\n%s\t%s\n",BDdevice->bdaddr,BDdevice->name);

        unsigned int filter = BD_ADDER|BD_NAME|BD_PAIRED|BD_ACLCONNECTED|BD_AVCONNECTED|BD_HFPCONNECTED|BD_UUID_PROFILES;

        json_object *jresp = new_json_object_from_device(BDdevice, filter);

        json_object_array_add(my_array, jresp);
    }

    adapter_devices_list_free(list);

    json_object_object_add(jresp, "list", my_array);

    afb_req_success(request, jresp, "BT - Scan Result is Displayed");
}

/**/
static void bt_pair (struct afb_req request)
{
    LOGD("\n");

    const char *value = afb_req_value (request, "value");
    int ret = 0;

    if (NULL == value)
    {
        afb_req_fail (request, "failed", "Please Input the Device Address");
        return;
    }

    ret = device_pair(value);
    if (0 == ret)
    {
        afb_req_success (request, NULL, NULL);
    }
    else
    {
        afb_req_fail (request, "failed", "Device pairing failed");
    }

}

/**/
static void bt_cancel_pairing (struct afb_req request)
{
    LOGD("\n");

    const char *value = afb_req_value (request, "value");
    int ret = 0;

    if (NULL == value)
    {
        afb_req_fail (request, "failed", "Please Input the Device Address");
        return;
    }

    ret = device_cancelPairing(value);
    if (0 == ret)
    {
        afb_req_success (request, NULL, NULL);
    }
    else
    {
        afb_req_fail (request, "failed", "Device cancel pairing failed");
    }

}

/**/
static void bt_connect (struct afb_req request)
{
    LOGD("\n");

    const char *value = afb_req_value (request, "value");
    const char *uuid = afb_req_value (request, "uuid");
    int ret = 0;

    if (NULL == value)
    {
        afb_req_fail (request, "failed", "Please Input the Device Address");
        return;
    }

    ret = device_connect(value, uuid);

    if (0 == ret)
    {
        json_object *jresp = json_object_new_object();
        json_object *jstring;

        jstring = json_object_new_string("connected");
        json_object_object_add(jresp, "Status", jstring);

        jstring = json_object_new_string(value);
        json_object_object_add(jresp, "Address", jstring);

        if (uuid) {
            jstring = json_object_new_string(uuid);
            json_object_object_add(jresp, "UUID", jstring);
        }

        event_push(jresp, "connection");
        afb_req_success (request, NULL, NULL);
    }
    else
    {
        afb_req_fail (request, "failed", "Device connect failed");
    }

}

/**/
static void bt_disconnect (struct afb_req request)
{
    LOGD("\n");

    const char *value = afb_req_value (request, "value");
    const char *uuid = afb_req_value (request, "uuid");
    int ret = 0;

    if (NULL == value)
    {
        afb_req_fail (request, "failed", "Please Input the Device Address");
        return;
    }

    ret = device_disconnect(value, uuid);
    if (0 == ret)
    {
        json_object *jresp = json_object_new_object();
        json_object *jstring;

        jstring = json_object_new_string("disconnected");
        json_object_object_add(jresp, "Status", jstring);

        jstring = json_object_new_string(value);
        json_object_object_add(jresp, "Address", jstring);

        if (uuid) {
            jstring = json_object_new_string(uuid);
            json_object_object_add(jresp, "UUID", jstring);
        }

        event_push(jresp, "connection");
        afb_req_success (request, NULL, NULL);
    }
    else
    {
        afb_req_fail (request, "failed", "Device disconnect failed");
    }

}

/**/
static void bt_remove_device (struct afb_req request)
{
    LOGD("\n");

    const char *value = afb_req_value (request, "value");
    int ret = 0;

    if (NULL == value)
    {
        afb_req_fail (request, "failed", "Please Input the Device Address");
        return;
    }

    ret = adapter_remove_device(value);
    if (0 == ret)
    {
                afb_req_success (request, NULL, NULL);
    }
    else
    {
        afb_req_fail (request, "failed", "Remove Device failed");
    }

}

static void bt_device_priorities_cb (json_object *array, gchar *str)
{
    json_object_array_add(array, json_object_new_string(str));
}

static void bt_device_priorities (struct afb_req request)
{
    json_object *jarray = json_object_new_array();

    device_priority_list(&bt_device_priorities_cb, jarray);

    afb_req_success(request, jarray, "Display paired device priority");
}

/**/
static void bt_set_device_property (struct afb_req request)
{
    LOGD("\n");

    const char *address = afb_req_value (request, "Address");
    const char *property = afb_req_value (request, "Property");
    const char *value = afb_req_value (request, "value");
    int ret = 0;
    GSList *list = NULL;

    if (NULL == address || NULL==property || NULL==value)
    {
        afb_req_fail (request, "failed", "Please Check Input Parameter");
        return;
    }

    ret = device_set_property(address, property, value);
    if (0 == ret)
    {
        afb_req_success (request, NULL, NULL);
    }
    else
    {
        afb_req_fail (request, "failed", "Device set property failed");
    }

}

/**/
static void bt_set_property (struct afb_req request)
{
    LOGD("\n");

    const char *property = afb_req_value (request, "Property");
    const char *value = afb_req_value (request, "value");
    int ret = 0;
    gboolean setvalue;


    if (NULL==property || NULL==value)
    {
        afb_req_fail (request, "failed", "Please Check Input Parameter");
        return;
    }


    if ( atoi(value) == 1 || !strcasecmp(value, "true") )
    {
        ret = adapter_set_property (property, TRUE);

    }

    /* "?value=" parameter is "0" or "false" */
    else if ( atoi(value) == 0 || !strcasecmp(value, "false") )
    {
        ret = adapter_set_property (property, FALSE);
    }
    else
    {
        afb_req_fail (request, "failed", "Invalid value");
        return;
    }

    if (0 == ret)
    {
        afb_req_success (request, NULL, NULL);
    }
    else
    {
        afb_req_fail (request, "failed", "Bluetooth set property failed");
    }

}

/**/
static void bt_set_avrcp_controls (struct afb_req request)
{
    LOGD("\n");

    const char *address = afb_req_value (request, "Address");
    const char *value = afb_req_value (request, "value");
    int ret = 0;
    GSList *list = NULL;

    if (NULL==value)
    {
        afb_req_fail (request, "failed", "Please Check Input Parameter");
        return;
    }

    if (NULL == address)
    {
        list = adapter_get_devices_list();
        if (NULL == list)
        {
            afb_req_fail (request, "failed", "No find devices");
            return;
        }

        for (;list;list=list->next)
        {
            struct btd_device *BDdevice = list->data;
            //LOGD("\n%s\t%s\n",BDdevice->bdaddr,BDdevice->name);
            if (BDdevice->avconnected)
            {
                address = BDdevice->bdaddr;
                break;
            }
        }
    }

    ret = device_call_avrcp_method(address, value);
    if (0 == ret)
    {
        afb_req_success (request, NULL, NULL);
    }
    else
    {
        afb_req_fail (request, "failed", "Bluetooth set avrcp control failed");
    }
}

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
 * broadcast new device
 */
void bt_broadcast_device_added(struct btd_device *BDdevice)
{
    unsigned int filter = BD_ADDER|BD_NAME|BD_PAIRED|BD_ACLCONNECTED|BD_AVCONNECTED|BD_HFPCONNECTED|BD_UUID_PROFILES;
    int ret;
    json_object *jresp = new_json_object_from_device(BDdevice, filter);

    LOGD("\n");
    ret = event_push(jresp,"device_added");
    LOGD("%d\n",ret);
}

/*
 * broadcast device removed
 */
void bt_broadcast_device_removed(struct btd_device *BDdevice)
{
    unsigned int filter = BD_ADDER;
    int ret;
    json_object *jresp = new_json_object_from_device(BDdevice, filter);

    LOGD("\n");
    ret = event_push(jresp,"device_removed");
    LOGD("%d\n",ret);
}


/*
 * broadcast device updated
 */
void bt_broadcast_device_properties_change(struct btd_device *BDdevice)
{

    unsigned int filter = BD_ADDER|BD_NAME|BD_PAIRED|BD_ACLCONNECTED|BD_AVCONNECTED|BD_HFPCONNECTED|BD_AVRCP_TITLE|BD_AVRCP_ARTIST|BD_AVRCP_STATUS|BD_AVRCP_DURATION|BD_AVRCP_POSITION|BD_TRANSPORT_STATE|BD_TRANSPORT_VOLUME|BD_UUID_PROFILES;

    int ret;
    json_object *jresp = new_json_object_from_device(BDdevice, filter);

    LOGD("\n");
    ret = event_push(jresp,"device_updated");
    LOGD("%d\n",ret);
}

/*
 * broadcast request confirmation
 */
gboolean bt_request_confirmation(const gchar *device, guint passkey)
{
    json_object *jresp = json_object_new_object();
    json_object *jstring = NULL;
    int ret;

    jstring = json_object_new_string(device);
    json_object_object_add(jresp, "Address", jstring);
    jstring = json_object_new_int(passkey);
    json_object_object_add(jresp, "Passkey", jstring);

    ret = event_push(jresp,"request_confirmation");

    LOGD("%d\n",ret);

    if (ret >0) {
        return TRUE;
    }else {
        return FALSE;
    }

}

static void bt_send_confirmation(struct afb_req request)
{
    const char *value = afb_req_value(request, "value");
    int ret;
    gboolean confirmation;

    if (!value) {
            afb_req_fail (request, "failed", "Unable to get value status");
            return;
    }

    /* "?value=" parameter is "1" or "yes" */
    else if ( atoi(value) == 1 || !strcasecmp(value, "yes") )
    {
        ret = agent_send_confirmation (TRUE);
    }

    /* "?value=" parameter is "0" or "no" */
    else if ( atoi(value) == 0 || !strcasecmp(value, "no") )
    {
        ret = agent_send_confirmation (TRUE);
    }
    else
    {
        afb_req_fail (request, "failed", "Invalid value");
        return;
    }

    if ( 0==ret) {
        afb_req_success(request, NULL, NULL);
    }else {
        afb_req_success(request, "failed", "fail");
    }

}

/*
 * array of the verbs exported to afb-daemon
 */
static const struct afb_verb_v2 binding_verbs[]= {
/* VERB'S NAME                    FUNCTION TO CALL                    SHORT DESCRIPTION */
{ .verb = "power",               .callback = bt_power,               .info = "Set Bluetooth Power ON or OFF" },
{ .verb = "start_discovery",     .callback = bt_start_discovery,     .info = "Start discovery" },
{ .verb = "stop_discovery",      .callback = bt_stop_discovery,      .info = "Stop discovery" },
{ .verb = "discovery_result",    .callback = bt_discovery_result,    .info = "Get discovery result" },
{ .verb = "remove_device",       .callback = bt_remove_device,       .info = "Remove the special device" },
{ .verb = "pair",                .callback = bt_pair,                .info = "Pair to special device" },
{ .verb = "cancel_pair",         .callback = bt_cancel_pairing,      .info = "Cancel the pairing process" },
{ .verb = "connect",             .callback = bt_connect,             .info = "Connect to special device" },
{ .verb = "disconnect",          .callback = bt_disconnect,          .info = "Disconnect special device" },
{ .verb = "device_priorities",	 .callback = bt_device_priorities,   .info = "Get BT paired device priorites" },
{ .verb = "set_device_property", .callback = bt_set_device_property, .info = "Set special device property" },
{ .verb = "set_property",        .callback = bt_set_property,        .info = "Set Bluetooth property" },
{ .verb = "set_avrcp_controls",  .callback = bt_set_avrcp_controls,  .info = "Set Bluetooth AVRCP controls" },
{ .verb = "send_confirmation",   .callback = bt_send_confirmation,   .info = "Send Confirmation" },
{ .verb = "subscribe",           .callback = subscribe,              .info = "subscribes to the event of 'value'"},
{ .verb = "unsubscribe",         .callback = unsubscribe,            .info = "unsubscribes to the event of 'value'"},

{ } /* marker for end of the array */
};

/*
 * activation function for registering the binding called by afb-daemon
 */

static int init()
{
    Binding_RegisterCallback_t API_Callback;
    API_Callback.binding_device_added = bt_broadcast_device_added;
    API_Callback.binding_device_removed = bt_broadcast_device_removed;
    API_Callback.binding_device_properties_changed = bt_broadcast_device_properties_change;
    API_Callback.binding_request_confirmation = bt_request_confirmation;
    BindingAPIRegister(&API_Callback);

    // register binding events
    event_add("connection");
    event_add("request_confirmation");
    event_add("device_added");
    event_add("device_removed");
    event_add("device_updated");

    return BluetoothManagerInit();
}

/*
 * description of the binding for afb-daemon
 */
const struct afb_binding_v2 afbBindingV2 =
{
    .specification    = "Application Framework Binder - Bluetooth Manager plugin",
    .api              = "Bluetooth-Manager",
    .verbs            = binding_verbs,
    .init             = init,
};

/***************************** The End Of File ******************************/

