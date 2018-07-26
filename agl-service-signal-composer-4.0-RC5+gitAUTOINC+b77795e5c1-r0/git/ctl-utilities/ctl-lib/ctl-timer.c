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
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "ctl-config.h"
#include "ctl-timer.h"

#define DEFAULT_PAUSE_DELAY 3000
#define DEFAULT_TEST_COUNT 1
typedef struct {
    int value;
    const char *uid;
} AutoTestCtxT;


STATIC int TimerNext (sd_event_source* source, uint64_t timer, void* handle) {
    TimerHandleT *timerHandle = (TimerHandleT*) handle;
    int done;
    uint64_t usec;

    done= timerHandle->callback(timerHandle);
    if (!done) goto OnErrorExit;

    // Rearm timer if needed
    timerHandle->count --;
    if (timerHandle->count == 0) {
        sd_event_source_unref(source);
        if (timerHandle->freeCB) timerHandle->freeCB(timerHandle->context);
        free (handle);
        return 0;
    }
    else {
        // otherwise validate timer for a new run
        sd_event_now(AFB_GetEventLoop(timerHandle->api), CLOCK_MONOTONIC, &usec);
        sd_event_source_set_enabled(source, SD_EVENT_ONESHOT);
        sd_event_source_set_time(source, usec + timerHandle->delay*1000);
    }

    return 0;

OnErrorExit:
    AFB_ApiWarning(timerHandle->api, "TimerNext Callback Fail Tag=%s", timerHandle->uid);
    return -1;
}

PUBLIC void TimerEvtStop(TimerHandleT *timerHandle) {

    sd_event_source_unref(timerHandle->evtSource);
    free (timerHandle);
}


PUBLIC void TimerEvtStart(AFB_ApiT apiHandle, TimerHandleT *timerHandle, timerCallbackT callback, void *context) {
    uint64_t usec;

    // populate CB handle
    timerHandle->callback=callback;
    timerHandle->context=context;
    timerHandle->api=apiHandle;

    // set a timer with ~250us accuracy
    sd_event_now(AFB_GetEventLoop(apiHandle), CLOCK_MONOTONIC, &usec);
    sd_event_add_time(AFB_GetEventLoop(apiHandle), &timerHandle->evtSource, CLOCK_MONOTONIC, usec+timerHandle->delay*1000, 250, TimerNext, timerHandle);
}


// Create Binding Event at Init
PUBLIC int TimerEvtInit (AFB_ApiT apiHandle) {

    AFB_ApiDebug (apiHandle, "Timer-Init Done");
    return 0;
}

