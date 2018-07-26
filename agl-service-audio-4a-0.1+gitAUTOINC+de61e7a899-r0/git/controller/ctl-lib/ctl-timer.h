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


#ifndef CTL_TIMER_INCLUDE
#define CTL_TIMER_INCLUDE

#ifdef __cplusplus
extern "C" {
#endif

#include <systemd/sd-event.h>

// ctl-timer.c
// ----------------------

typedef struct TimerHandleS {
    int magic;
    int count;
    int delay;
    const char*uid;
    void *context;
    sd_event_source *evtSource;
    AFB_ApiT api;
    int (*callback) (struct TimerHandleS *handle);
    int (*freeCB) (void *context) ;
} TimerHandleT;

typedef int (*timerCallbackT)(TimerHandleT *context);

int TimerEvtInit (AFB_ApiT apiHandle);
void TimerEvtStart(AFB_ApiT apiHandle, TimerHandleT *timerHandle, timerCallbackT callback, void *context);
void TimerEvtStop(TimerHandleT *timerHandle);

uint64_t LockWait(AFB_ApiT apiHandle, uint64_t utimeout);
#ifdef __cplusplus
}
#endif

#endif // CTL_TIMER_INCLUDE
