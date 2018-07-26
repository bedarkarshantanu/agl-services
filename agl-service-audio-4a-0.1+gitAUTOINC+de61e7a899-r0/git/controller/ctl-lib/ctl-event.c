/*
 * Copyright (C) 2016 "IoT.bzh"
 * Author Fulup Ar Foll <fulup@iot.bzh>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, something express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>

#include "ctl-config.h"


// Event dynamic API-V3 mode
#ifdef AFB_BINDING_PREV3
void CtrlDispatchApiEvent (AFB_ApiT apiHandle, const char *evtLabel, struct json_object *eventJ) {
    int idx = 0;
    CtlActionT* actions = NULL;
    AFB_ApiDebug (apiHandle, "Received event=%s, query=%s", evtLabel, json_object_get_string(eventJ));

    // retrieve section config from api handle
    CtlConfigT *ctrlConfig = (CtlConfigT*) afb_dynapi_get_userdata(apiHandle);

    for (idx = 0; ctrlConfig->sections[idx].key != NULL; ++idx)
    {
        if(! strcasecmp(ctrlConfig->sections[idx].key, "events")) {
            actions = ctrlConfig->sections[idx].actions;
            break;
        }
    }

    idx = ActionLabelToIndex(actions, evtLabel);
    if (idx < 0) {
        AFB_ApiWarning(apiHandle, "CtlDispatchEvent: fail to find uid=%s in action event section", evtLabel);
        return;
    }

    // create a dummy source for action
    CtlSourceT source;
    source.uid = actions[idx].uid;
    source.api = actions[idx].api;
    source.request = NULL;

    // Best effort ignoring error to exec corresponding action
    (void) ActionExecOne (&source, &actions[idx], json_object_get(eventJ));

}
#else
// In API-V2 controller config is unique and static
extern CtlConfigT *ctrlConfig;

// call action attached to even name if any
void CtrlDispatchV2Event(const char *evtLabel, json_object *eventJ) {
    CtlActionT* actions = ctrlConfig->sections[CTL_SECTION_EVENT].actions;

    int index= ActionLabelToIndex(actions, evtLabel);
    if (index < 0) {
        AFB_WARNING ("CtlDispatchEvent: fail to find uid=%s in action event section", evtLabel);
        return;
    }

    CtlSourceT source;
    source.uid = actions[index].uid;
    source.api   = actions[index].api;
    source.request = AFB_ReqNone;

    // Best effort ignoring error to exec corresponding action
    (void) ActionExecOne (&source, &actions[index], json_object_get(eventJ));
}
#endif

// onload section receive one action or an array of actions
int EventConfig(AFB_ApiT apiHandle, CtlSectionT *section, json_object *actionsJ) {

    // Load time parse actions in config file
    if (actionsJ != NULL) {
        section->actions= ActionConfig(apiHandle, actionsJ, 0);

        if (!section->actions) {
            AFB_ApiError (apiHandle, "EventLoad config fail processing onload actions");
            return 1;
        }
    }

    return 0;
}
