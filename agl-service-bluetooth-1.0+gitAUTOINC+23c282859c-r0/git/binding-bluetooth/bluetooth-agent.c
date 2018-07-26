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
#include <string.h>
#include <gio/gio.h>

#include "bluetooth-agent.h"

#ifdef AGENT_THREAD

#include <pthread.h>

static GMainLoop *agentLoop = NULL;
#endif

static GDBusConnection *system_conn = NULL;
static gboolean agent_registered = FALSE;
static const char *agent_capability = NULL;
static Agent_RegisterCallback_t agent_RegisterCallback = { 0 };
static AGENTOrgBluezAgent1 * agnet_interface = NULL;

static struct Agent_Node *agent_event = NULL;


static void agent_event_auto_release(struct Agent_Node *event)
{
    if (NULL == event ){
        return;
    }
    g_dbus_method_invocation_return_dbus_error (event->invocation,
                                                ERROR_BLUEZ_REJECT, "");
    g_free(event);
}

/*
 * Store the agent event
 */
static void agent_event_new(AGENTOrgBluezAgent1      *object,
                    GDBusMethodInvocation   *invocation, int type)
{
    LOGD("\n");
    if(NULL !=agent_event )
    {
        agent_event_auto_release(agent_event);
    }
    agent_event = g_malloc0(sizeof(struct Agent_Node));
    agent_event->type = type;
    agent_event->object = object;
    agent_event->invocation = invocation;

}


/*
 * agent api
 *
 * Methods : void Release()
 * This method gets called when the service daemon
 * unregisters the agent. An agent can use it to do
 * cleanup tasks. There is no need to unregister the
 * agent, because when this method gets called it has
 * already been unregistered.
 */
static gboolean
on_handle_Release (AGENTOrgBluezAgent1      *object,
                    GDBusMethodInvocation   *invocation,
                    gpointer                user_data)
{
    LOGD("\n");

    //gboolean ret = FALSE;

    if (NULL != agent_RegisterCallback.agent_Release)
    {
        agent_RegisterCallback.agent_Release();
    }

    agent_org_bluez_agent1_complete_release(object, invocation);

    g_dbus_interface_skeleton_unexport(
        G_DBUS_INTERFACE_SKELETON(agnet_interface));

    g_object_unref(agnet_interface);
    agent_registered = FALSE;

#ifdef AGENT_THREAD
    if(agentLoop){
        g_main_loop_quit(agentLoop);
    }
#endif

    return TRUE;
}

/*
 * agent api
 * Methods : string RequestPinCode(object device)
 *
 * This method gets called when the service daemon
 * needs to get the passkey for an authentication.
 *
 * The return value should be a string of 1-16 characters
 * length. The string can be alphanumeric.
 */
static gboolean
on_handle_RequestPinCode (AGENTOrgBluezAgent1       *object,
                            GDBusMethodInvocation   *invocation,
                            const gchar             *device,
                            gpointer                user_data)
{

    LOGD("device:%s.\n",device);

    gboolean ret = FALSE;
    const gchar *error = ERROR_BLUEZ_REJECT;
    gchar *response;

    if (NULL != agent_RegisterCallback.agent_RequestPinCode)
    {
        ret = agent_RegisterCallback.agent_RequestPinCode (device,
                                                           &response, &error);
    }

    if (TRUE == ret)
    {
        agent_org_bluez_agent1_complete_request_pin_code(object,
                                                        invocation, response);
        g_free(response);

    }
    else
    {
         g_dbus_method_invocation_return_dbus_error (invocation, error, "");
    }

    return TRUE;
}

/*
 * agent api
 * Methods : void DisplayPinCode(object device, string pincode)
 *
 * This method gets called when the service daemon
 * needs to display a pincode for an authentication.
 */
static gboolean
on_handle_DisplayPinCode (AGENTOrgBluezAgent1   *object,
                        GDBusMethodInvocation   *invocation,
                        const gchar             *device,
                        const gchar             *pincode,
                        gpointer                user_data)
{

    LOGD("device:%s,pincode:%s.\n",device, pincode);

    gboolean ret = FALSE;
    const gchar *error = ERROR_BLUEZ_REJECT;

    if (NULL != agent_RegisterCallback.agent_DisplayPinCode)
    {
        ret = agent_RegisterCallback.agent_DisplayPinCode(device,
                                                            pincode, &error);
    }

    if (TRUE == ret)
    {
        agent_org_bluez_agent1_complete_display_pin_code(object, invocation);
    }
    else
    {
        g_dbus_method_invocation_return_dbus_error (invocation, error, "");
    }

     return TRUE;
}


/*
 * agent api
 * Methods : uint32 RequestPasskey(object device)
 *
 * This method gets called when the service daemon
 * needs to get the passkey for an authentication.
 *
 * The return value should be a numeric value
 * between 0-999999.
 */
static gboolean
on_handle_RequestPasskey (AGENTOrgBluezAgent1   *object,
                        GDBusMethodInvocation   *invocation,
                        const gchar             *device,
                        gpointer                user_data)
{

    LOGD("device:%s.\n",device);

    gboolean ret = FALSE;
    const gchar *error = ERROR_BLUEZ_REJECT;
    guint passkey;

    if (NULL != agent_RegisterCallback.agent_RequestPasskey)
    {
        ret = agent_RegisterCallback.agent_RequestPasskey(device,
                                                            &passkey, &error);
    }

    if (TRUE == ret)
    {
        agent_org_bluez_agent1_complete_request_passkey(object,
                                                        invocation, passkey);
    }
    else
    {
        g_dbus_method_invocation_return_dbus_error (invocation, error, "");
    }

    return TRUE;

}


/*
 * agent api
 * Methods : void DisplayPasskey(object device, uint32 passkey, uint16 entered)
 *
 * This method gets called when the service daemon
 * needs to display a passkey for an authentication.
 *
 * The entered parameter indicates the number of already
 * typed keys on the remote side.
 */
static gboolean
on_handle_DisplayPasskey (AGENTOrgBluezAgent1   *object,
                        GDBusMethodInvocation   *invocation,
                        const gchar             *device,
                        guint                   passkey,
                        guint16                 entered,
                        gpointer                user_data)
{

    LOGD("device:%s,passkey:%d,entered:%d.\n",device, passkey, entered);

    //gboolean ret = FALSE;

    if (NULL != agent_RegisterCallback.agent_DisplayPasskey)
    {
        agent_RegisterCallback.agent_DisplayPasskey(device, passkey, entered);
    }

    agent_org_bluez_agent1_complete_display_passkey(object, invocation);


    return TRUE;

}

/*
 * agent api
 * Methods : void RequestConfirmation(object device, uint32 passkey)
 *
 * This method gets called when the service daemon
 * needs to confirm a passkey for an authentication.
 *
 * To confirm the value it should return an empty reply
 * or an error in case the passkey is invalid.
 */
static gboolean
on_handle_RequestConfirmation (AGENTOrgBluezAgent1  *object,
                            GDBusMethodInvocation   *invocation,
                            const gchar             *device,
                            guint                   passkey,
                            gpointer                user_data)
{
    LOGD("device:%s,passkey:%d.\n", device, passkey);

    gboolean ret = FALSE;
    const gchar *error = ERROR_BLUEZ_REJECT;

    if (NULL != agent_RegisterCallback.agent_RequestConfirmation)
    {
        ret = agent_RegisterCallback.agent_RequestConfirmation(device,
                                                              passkey, &error);
    }
    LOGD("ret %d\n",ret);
    if (TRUE == ret)
    {
        agent_event_new(object, invocation,REQUEST_CONFIRMATION);
        //agent_org_bluez_agent1_complete_request_confirmation(object,
        //                                                    invocation);
    }
    else
    {
        g_dbus_method_invocation_return_dbus_error (invocation, error, "");
    }

    return TRUE;
}

/*
 * agent api
 * Methods : void RequestAuthorization(object device)
 *
 * This method gets called to request the user to
 * authorize an incoming pairing attempt which
 * would in other circumstances trigger the just-works
 * model.
 */
static gboolean
on_handle_RequestAuthorization (AGENTOrgBluezAgent1     *object,
                                GDBusMethodInvocation   *invocation,
                                const gchar             *device,
                                gpointer                user_data)
{
    LOGD("device:%s.\n",device);

    gboolean ret = FALSE;
    const gchar *error = ERROR_BLUEZ_REJECT;

    if (NULL != agent_RegisterCallback.agent_RequestAuthorization)
    {
        ret = agent_RegisterCallback.agent_RequestAuthorization(device,&error);
    }

    if (TRUE == ret)
    {
        agent_org_bluez_agent1_complete_request_authorization(object,
                                                                invocation);
    }
    else
    {
        g_dbus_method_invocation_return_dbus_error (invocation, error, "");
    }

    return TRUE;
}

/*
 * agent api
 * Methods : void AuthorizeService(object device, string uuid)
 *
 * This method gets called when the service daemon
 * needs to authorize a connection/service request.
 */
static gboolean
on_handle_AuthorizeService (AGENTOrgBluezAgent1     *object,
                            GDBusMethodInvocation   *invocation,
                            const gchar             *device,
                            const gchar             *uuid,
                            gpointer                user_data)
{

    LOGD("device:%s.uuid:%s\n",device,uuid);

    gboolean ret = FALSE;
    const gchar *error = ERROR_BLUEZ_REJECT;

    if (NULL != agent_RegisterCallback.agent_AuthorizeService)
    {
        ret = agent_RegisterCallback.agent_AuthorizeService(device,
                                                            uuid, &error);
    }

    if (TRUE == ret)
    {
        agent_org_bluez_agent1_complete_authorize_service(object, invocation);
    }
    else
    {
        g_dbus_method_invocation_return_dbus_error (invocation, error, "");
    }

    return TRUE;
}

/*
 * agent api
 * Methods : void Cancel()
 *
 * This method gets called to indicate that the agent
 * request failed before a reply was returned.
 */
static gboolean
on_handle_Cancel (AGENTOrgBluezAgent1           *object,
                       GDBusMethodInvocation    *invocation,
                       gpointer                 user_data)
{
    LOGD("\n");

    if (NULL != agent_RegisterCallback.agent_Cancel)
    {
        agent_RegisterCallback.agent_Cancel();
    }

    agent_org_bluez_agent1_complete_cancel(object, invocation);

    return TRUE;

}

/*
 * agent init
 * init the dbus and register the agent to bluez
 */
static int create_and_register_agent(const char *capability)
{
    GError *error = NULL;
    gboolean ret;
    GVariant *value;

    LOGD("%s\n",capability);

    if (agent_registered == TRUE) {
        LOGD("Agent is already registered\n");
        return -1;
    }

    system_conn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);

    if (error)
    {
        LOGE("errr:%s",error->message);
        g_error_free(error);

        return -1;
    }

    agent_capability = capability;

    agnet_interface = agent_org_bluez_agent1_skeleton_new ();

    g_signal_connect (agnet_interface,
                        "handle-release",
                        G_CALLBACK (on_handle_Release),
                        NULL);

    g_signal_connect (agnet_interface,
                        "handle-request-pin-code",
                        G_CALLBACK (on_handle_RequestPinCode),
                        NULL);

    g_signal_connect (agnet_interface,
                        "handle-display-pin-code",
                        G_CALLBACK (on_handle_DisplayPinCode),
                        NULL);

    g_signal_connect (agnet_interface,
                        "handle-request-passkey",
                        G_CALLBACK (on_handle_RequestPasskey),
                        NULL);

    g_signal_connect (agnet_interface,
                        "handle-display-passkey",
                        G_CALLBACK (on_handle_DisplayPasskey),
                        NULL);

    g_signal_connect (agnet_interface,
                        "handle-request-confirmation",
                        G_CALLBACK (on_handle_RequestConfirmation),
                        NULL);

    g_signal_connect (agnet_interface,
                        "handle-request-authorization",
                        G_CALLBACK (on_handle_RequestAuthorization),
                        NULL);

    g_signal_connect (agnet_interface,
                        "handle-authorize-service",
                        G_CALLBACK (on_handle_AuthorizeService),
                        NULL);

    g_signal_connect (agnet_interface,
                        "handle-cancel",
                        G_CALLBACK (on_handle_Cancel),
                        NULL);

    ret = g_dbus_interface_skeleton_export (
                                   G_DBUS_INTERFACE_SKELETON (agnet_interface),
                                    system_conn,
                                    AGENT_PATH,
                                    &error);

    if (FALSE == ret)
    {
        LOGE("errr:%s",error->message);
        g_error_free(error);
        g_object_unref(system_conn);

        return -1;
    }

    value = g_dbus_connection_call_sync(system_conn, BLUEZ_SERVICE,
                    AGENT_PATH,   AGENT_MANAGER_INTERFACE,
                    "RegisterAgent",    g_variant_new("(os)", AGENT_PATH,
                        agent_capability),
                    NULL,  G_DBUS_CALL_FLAGS_NONE, DBUS_REPLY_TIMEOUT,
                    NULL, &error);

    if (NULL == value) {
        LOGE ("RegisterAgent Err: %s", error->message);
        g_error_free(error);

        g_dbus_interface_skeleton_unexport(
            G_DBUS_INTERFACE_SKELETON(agnet_interface));

        g_object_unref(system_conn);
        return -1;
    }

    g_variant_unref(value);

    agent_capability = NULL;

    agent_registered = TRUE;

    return 0;
}

#ifdef AGENT_THREAD
/*
 * agent thread function
 */
static void *agent_event_loop_thread(const char *capability)
{

    agentLoop = g_main_loop_new(NULL, FALSE);
    guint id;
    GError *error = NULL;
    gboolean ret;

    ret = create_and_register_agent(capability);

    if (0 == ret)
    {
        LOGD("g_main_loop_run\n");
        g_main_loop_run(agentLoop);


    }

    g_main_loop_unref(agentLoop);
    agentLoop = NULL;
    LOGD("exit...\n");
}
#endif


/* --- PUBLIC FUNCTIONS --- */


/*
 * start agent.
 * Returns: 0 - success or other errors
 */
int agent_register(const char *capability)
{
    int ret = 0;
    LOGD("\n");

    if (agent_registered == TRUE) {
        LOGD("Agent is already registered\n");
        return -1;
    }

#ifdef AGENT_THREAD
    pthread_t thread_id;
    pthread_create(&thread_id, NULL, agent_event_loop_thread, capability);
    pthread_setname_np(thread_id, "agent");

#else
    ret = create_and_register_agent(capability);
#endif

    return ret;
}

/*
 * stop agent.
 * Returns: 0 - success or other errors
 */
int stop_agent()
{
    GError *error = NULL;
    GVariant *value;

    LOGD("\n");

    if (agent_registered == FALSE) {
        LOGD("No agent is registered\n");
        return -1;
    }

    value = g_dbus_connection_call_sync(system_conn, BLUEZ_SERVICE,
                    AGENT_PATH,   AGENT_MANAGER_INTERFACE,
                    "UnregisterAgent",    g_variant_new("(o)", AGENT_PATH  ),
                    NULL,  G_DBUS_CALL_FLAGS_NONE, DBUS_REPLY_TIMEOUT,
                    NULL, &error);

    if (NULL == value) {
        LOGE ("Error UnregisterAgent: %s", error->message);
        g_error_free(error);

        return -1;
    }

    g_dbus_interface_skeleton_unexport(
        G_DBUS_INTERFACE_SKELETON(agnet_interface));

    g_object_unref(agnet_interface);

    g_object_unref(system_conn);

    agent_registered = FALSE;

    memset(&agent_RegisterCallback, 0, sizeof(Agent_RegisterCallback_t));

#ifdef AGENT_THREAD
    if(agentLoop){
        g_main_loop_quit(agentLoop);
    }
#endif

    return 0;
}

/*
 * Register Agent Callback function
 */
void agent_API_register(const Agent_RegisterCallback_t* pstRegisterCallback)
{
    LOGD("\n");

    if (NULL != pstRegisterCallback)
    {
        if (NULL != pstRegisterCallback->agent_Release)
        {
            agent_RegisterCallback.agent_Release =
                pstRegisterCallback->agent_Release;
        }

        if (NULL != pstRegisterCallback->agent_RequestPinCode)
        {
            agent_RegisterCallback.agent_RequestPinCode =
                pstRegisterCallback->agent_RequestPinCode;
        }

        if (NULL != pstRegisterCallback->agent_DisplayPinCode)
        {
            agent_RegisterCallback.agent_DisplayPinCode =
                pstRegisterCallback->agent_DisplayPinCode;
        }

        if (NULL != pstRegisterCallback->agent_RequestPasskey)
        {
            agent_RegisterCallback.agent_RequestPasskey =
                pstRegisterCallback->agent_RequestPasskey;
        }

        if (NULL != pstRegisterCallback->agent_DisplayPasskey)
        {
            agent_RegisterCallback.agent_DisplayPasskey =
                pstRegisterCallback->agent_DisplayPasskey;
        }

        if (NULL != pstRegisterCallback->agent_RequestConfirmation)
        {
            agent_RegisterCallback.agent_RequestConfirmation =
                pstRegisterCallback->agent_RequestConfirmation;
        }

        if (NULL != pstRegisterCallback->agent_RequestAuthorization)
        {
            agent_RegisterCallback.agent_RequestAuthorization =
                pstRegisterCallback->agent_RequestAuthorization;
        }

        if (NULL != pstRegisterCallback->agent_AuthorizeService)
        {
            agent_RegisterCallback.agent_AuthorizeService =
                pstRegisterCallback->agent_AuthorizeService;
        }

        if (NULL != pstRegisterCallback->agent_Cancel)
        {
            agent_RegisterCallback.agent_Cancel =
                pstRegisterCallback->agent_Cancel;
        }

    }

}

/*
 * Send the agent event "RequestConfirmation" reply
 */
int agent_send_confirmation(gboolean confirmation)
{
    if (NULL == agent_event)
    {
        LOGW("Not agent event");
        return -1;
    }
    LOGD("%d-%d\n", confirmation, agent_event->type);

    if (REQUEST_CONFIRMATION != agent_event->type)
    {
        return -1;
    }

    if (TRUE == confirmation){
        agent_org_bluez_agent1_complete_request_confirmation(agent_event->object,
                                                      agent_event->invocation);
    }else{
        g_dbus_method_invocation_return_dbus_error (agent_event->invocation,
                                                    ERROR_BLUEZ_REJECT, "");
    }

    g_free(agent_event);
    agent_event = NULL;
    return 0;
}
/****************************** The End Of File ******************************/

