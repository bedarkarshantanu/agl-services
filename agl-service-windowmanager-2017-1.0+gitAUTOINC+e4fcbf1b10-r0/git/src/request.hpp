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

#ifndef WMREQUEST_HPP
#define WMREQUEST_HPP

#include <string>
#include <vector>

namespace wm
{

enum Task
{
    TASK_ALLOCATE,
    TASK_RELEASE,
    TASK_INVALID
};

enum TaskVisible
{
    VISIBLE,
    INVISIBLE,
    NO_CHANGE
};

struct WMTrigger
{
    std::string appid;
    std::string role;
    std::string area;
    Task task;
};

struct WMAction
{
    std::string appid;
    std::string role;
    std::string area;
    TaskVisible visible;
    bool end_draw_finished;
};

struct WMRequest
{
    WMRequest();
    explicit WMRequest(std::string appid, std::string role,
                       std::string area, Task task);
    virtual ~WMRequest();
    WMRequest(const WMRequest &obj);

    unsigned req_num;
    struct WMTrigger trigger;
    std::vector<struct WMAction> sync_draw_req;
};

} // namespace wm

#endif //WMREQUEST_HPP