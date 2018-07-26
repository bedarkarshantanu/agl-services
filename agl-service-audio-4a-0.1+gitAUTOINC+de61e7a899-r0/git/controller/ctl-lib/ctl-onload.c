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

// onload section receive one action or an array of actions
int OnloadConfig(AFB_ApiT apiHandle, CtlSectionT *section, json_object *actionsJ) {
    int err = 0;

    // Load time parse actions in control file
    if (actionsJ != NULL) {
        section->actions= ActionConfig(apiHandle, actionsJ, 0);

        if (!section->actions) {
            AFB_ApiError (apiHandle, "OnloadConfig control fail processing onload actions");
            return 1;
        }

    } else {
        // Exec time process onload action now
        if (!section->actions) {
            AFB_ApiError (apiHandle, "OnloadConfig Cannot Exec Non Existing Onload Action");
            return 1;
        }

        for (int idx=0; section->actions[idx].uid != NULL; idx ++) {
            CtlSourceT source;
            source.uid = section->actions[idx].uid;
            source.api  = section->actions[idx].api;
            source.request = AFB_ReqNone;

            if(!err)
                err = ActionExecOne(&source, &section->actions[idx], NULL);
            else {
                AFB_ApiError(apiHandle, "Onload action execution failed on: %s", source.uid);
                return err;
            }
        }
    }

    return err;
}
