/*
 * Copyright (c) 2017 TOYOTA MOTOR CORPORATION
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef WINDOW_MANAGER_ERROR
#define WINDOW_MANAGER_ERROR

namespace wm {

typedef enum WINDOWMANAGER_ERROR
{
    SUCCESS = 0,
    FAIL,
    REQ_REJECTED,
    REQ_DROPPED,
    TIMEOUT_EXPIRED,
    NOT_REGISTERED,
    LAYOUT_CHANGE_FAIL,
    NO_ENTRY,
    NO_LAYOUT_CHANGE,
    UNKNOWN,
    ERR_MAX = UNKNOWN
}
WMError;

const char *errorDescription(WMError enum_error_number);

}
#endif // WINDOW_MANAGER_ERROR