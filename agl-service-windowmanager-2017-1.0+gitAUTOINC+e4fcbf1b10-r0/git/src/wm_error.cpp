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
#include "wm_error.hpp"

namespace wm {

const char *errorDescription(WMError enum_error_number)
{
    switch (enum_error_number){
        case SUCCESS:
            return "Success";
        case FAIL:
            return "Request failed";
        case REQ_REJECTED:
            return "Request is rejected, due to the policy rejection of the request.";
        case REQ_DROPPED:
            return "Request is dropped, because the high priority request is done";
        case NOT_REGISTERED:
            return "Not registered";
        case TIMEOUT_EXPIRED:
            return "Request is dropped, due to time out expiring";
        case LAYOUT_CHANGE_FAIL:
            return "Layout change fails, due to some reasons";
        case NO_ENTRY:
            return "No element";
        case NO_LAYOUT_CHANGE:
            return "No layout change(deactivate only)";
        default:
            return "Unknown error number. Window manager bug.";
    }
}

}