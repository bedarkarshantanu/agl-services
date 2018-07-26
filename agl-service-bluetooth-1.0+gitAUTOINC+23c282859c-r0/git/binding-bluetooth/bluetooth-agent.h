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


#ifndef BLUETOOTH_AGENT_H
#define BLUETOOTH_AGENT_H

#include <glib.h>
#include <gio/gio.h>
#include <glib-object.h>

#include "lib_agent.h"
#include "bluetooth-manager.h"

//#define AGENT_THREAD
#define REQUEST_PINCCODE 1
#define REQUEST_PASSKEY 2
#define REQUEST_CONFIRMATION 3
#define REQUEST_AUTHORIZATION 4

struct Agent_Node
{
    int    type;
    AGENTOrgBluezAgent1     *object;
    GDBusMethodInvocation   *invocation;
};

typedef struct tagAgent_RegisterCallback
{
    void (*agent_Release)(void);
    gboolean (*agent_RequestPinCode) (const gchar *device, gchar **pincode, const gchar **error);
    gboolean (*agent_DisplayPinCode)(const gchar *device, const gchar *pincode, const gchar **error);
    gboolean (*agent_RequestPasskey)(const gchar *device, guint *passkey, const gchar **error);
    void (*agent_DisplayPasskey)(const gchar *device, guint passkey, guint16 entered);
    gboolean (*agent_RequestConfirmation)(const gchar *device, guint passkey, const gchar **error);
    gboolean (*agent_RequestAuthorization)(const gchar *device, const gchar **error);
    gboolean (*agent_AuthorizeService)(const gchar *device, const gchar *uuid, const gchar **error);
    void (*agent_Cancel)(void);
}Agent_RegisterCallback_t;



/* --- PUBLIC FUNCTIONS --- */
void agent_API_register(const Agent_RegisterCallback_t* pstRegisterCallback);

int agent_register(const char *capability);
int stop_agent();

int agent_send_confirmation(gboolean confirmation);

#endif /* BLUETOOTH_AGENT_H */


/****************************** The End Of File ******************************/

