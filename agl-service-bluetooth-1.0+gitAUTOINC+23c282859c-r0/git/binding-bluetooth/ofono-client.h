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


#ifndef OFONO_CLIENT_H
#define OFONO_CLIENT_H

#include <glib.h>
#include <gio/gio.h>
#include <glib-object.h>

#include "lib_ofono.h"
#include "lib_ofono_modem.h"

#include "bluetooth-manager.h"

//#define OFONO_THREAD

struct ofono_modem {
    gchar   *path;
    OFONOMODEMOrgOfonoModem *proxy;

    //gboolean online;
    gboolean powered;
    //gboolean lockdown;
    //gboolean emergency;
    //GVariant *interfaces;
    //GVariant *features;
    //gchar   *name;
    //gchar   *type;

};

typedef struct {
    gboolean inited;
    GMutex m;
    GSList * modem;
    OFONOOrgOfonoManager *proxy;
    GDBusConnection *system_conn;
} stOfonoManager;

typedef struct tagOfono_RegisterCallback
{
    void (*modem_added)(struct ofono_modem *modem);
    void (*modem_removed)(struct ofono_modem *modem);
    void (*modem_properties_changed)(struct ofono_modem *modem);
}Ofono_RegisterCallback_t;

/* --- PUBLIC FUNCTIONS --- */
void OfonoModemAPIRegister(const Ofono_RegisterCallback_t* pstRegisterCallback);

int OfonoManagerInit(void) ;
int OfonoManagerQuit(void) ;

gboolean getOfonoModemPoweredByPath (gchar* path);


#endif /* OFONO_CLIENT_H */


/****************************** The End Of File ******************************/

