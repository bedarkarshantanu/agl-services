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

#include "request.hpp"

namespace wm
{

using std::string;

WMRequest::WMRequest() {}

WMRequest::WMRequest(string appid, string role, string area, Task task)
    : req_num(0),
      trigger{appid, role, area, task},
      sync_draw_req(0)
{
}

WMRequest::~WMRequest()
{
}

WMRequest::WMRequest(const WMRequest &obj)
{
    this->req_num = obj.req_num;
    this->trigger = obj.trigger;
    this->sync_draw_req = obj.sync_draw_req;
}

} // namespace wm