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
#include <iostream>
#include <algorithm>
#include "applist.hpp"
#include "../include/hmi-debug.h"

using std::shared_ptr;
using std::string;
using std::vector;

namespace wm
{

const static int kReserveClientSize = 100;
const static int kReserveReqSize    = 10;

/**
 * AppList Constructor.
 *
 * Reserve the container size to avoid re-allocating memory.
 *
 * @note Size should be changed according to the system.
 *       If the number of applications becomes over the size, re-allocating memory will happen.
 */
AppList::AppList()
    : req_list(),
      app2client(),
      current_req(1)
{
    this->app2client.reserve(kReserveClientSize);
    this->req_list.reserve(kReserveReqSize);
}

AppList::~AppList() {}

// =================== Client Date container API ===================

/**
 * Add Client to the list
 *
 * Add Client to the list.
 * The Client means application which has role, layer, surface
 * This function should be called once for the app.
 * Caller should take care not to be called more than once.
 *
 * @param     string[in]   Application id. This will be the key to withdraw the information.
 * @param     unsigned[in] Layer ID in which the application is
 * @param     unsigned[in] surface ID which the application has
 * @param     string[in]   Role which means what behavior the application will do.
 * @return    None
 * @attention This function should be called once for the app
 *            Caller should take care not to be called more than once.
 */
void AppList::addClient(const std::string &appid, unsigned layer, unsigned surface, const std::string &role)
{
    std::lock_guard<std::mutex> lock(this->mtx);
    shared_ptr<WMClient> client = std::make_shared<WMClient>(appid, layer, surface, role);
    this->app2client[appid] = client;
    this->clientDump();
}

/**
 * Remove WMClient from the list
 *
 * @param string[in] Application id. This will be the key to withdraw the information.
 */
void AppList::removeClient(const string &appid)
{
    std::lock_guard<std::mutex> lock(this->mtx);
    this->app2client.erase(appid);
    HMI_INFO("wm", "Remove client %s", appid.c_str());
}

/**
 * Check this class stores the appid.
 *
 * @param     string[in] Application id. This will be the key to withdraw the information.
 * @return    true if the class has the requested key(appid)
 */
bool AppList::contains(const string &appid) const
{
    auto result = this->app2client.find(appid);
    return (this->app2client.end() != result) ? true : false;
}

/**
 * Remove surface from client
 *
 * @param     unsigned[in] surface id.
 * @return    None
 */
void AppList::removeSurface(unsigned surface){
    // This function may be very slow
    std::lock_guard<std::mutex> lock(this->mtx);
    bool ret = false;
    for (auto &x : this->app2client)
    {
        ret = x.second->removeSurfaceIfExist(surface);
        if(ret){
            HMI_DEBUG("wm", "remove surface %d from Client %s finish",
                        surface, x.second->appID().c_str());
            break;
        }
    }

}

/**
 * Get WMClient object.
 *
 * After get the WMClient object, caller can call the client method.
 * Before call this function, caller must call "contains"
 * to check the key is contained, otherwise, you have to take care of std::out_of_range.
 *
 * @param     string[in] application id(key)
 * @return    WMClient object
 * @attention Must call cantains to check appid is stored before this function.
 */
shared_ptr<WMClient> AppList::lookUpClient(const string &appid)
{
    return this->app2client.at(appid);
}

/**
 * Count Client.
 *
 * Returns the number of client stored in the list.
 *
 * @param  None
 * @return The number of client
 */
int AppList::countClient() const
{
    return this->app2client.size();
}

/**
 * Get AppID with surface and role.
 *
 * Returns AppID if found.
 *
 * @param     unsigned[in] surfaceID
 * @param     string[in]   role
 * @param     bool[in,out] AppID is found or not
 * @return    AppID
 * @attention If AppID is not found, param found will be false.
 */
string AppList::getAppID(unsigned surface, const string& role, bool* found) const
{
    *found = false;
    for (const auto &x : this->app2client)
    {
        if(x.second->surfaceID(role) == surface){
            *found = true;
            return x.second->appID();
        }
    }
    return string("");
}

// =================== Request Date container API ===================

/**
 * Get current request number
 *
 * Request number is the numeric ID to designate the request.
 * But returned request number from this function doesn't mean the request exists.
 * This number is used as key to withdraw the WMRequest object.
 *
 * @param  None
 * @return current request number.
 * @note   request number is more than 0.
 */
unsigned AppList::currentRequestNumber() const
{
    return this->current_req;
}

/**
 * Get request number
 *
 * Request number is the numeric ID to designate the request.
 * But returned request number from this function doesn't mean the request exists.
 * This number is used as key to withdraw the WMRequest object.
 *
 * @param     None
 * @return    request number.
 * @attention If returned value is 0, no request exists.
 */
unsigned AppList::getRequestNumber(const string &appid) const
{
    for (const auto &x : this->req_list)
    {
        // Since app will not request twice and more, comparing appid is enough?
        if ((x.trigger.appid == appid))
        {
            return x.req_num;
        }
    }
    return 0;
}

/**
 * Add Request
 *
 * Request number is the numeric ID to designate the request.
 * But returned request number from this function doesn't mean the request exists.
 * This number is used as key to withdraw the WMRequest object.
 *
 * @param     WMRequest[in] WMRequest object caller creates
 * @return    Request number
 * @attention If the request number is different with curent request number,
 *            it means the previous request is not finished.
 */
unsigned AppList::addRequest(WMRequest req)
{
    std::lock_guard<std::mutex> lock(this->mtx);
    if (this->req_list.size() == 0)
    {
        req.req_num = current_req;
    }
    else
    {
        HMI_SEQ_INFO(this->current_req, "add: %d", this->req_list.back().req_num + 1);
        req.req_num = this->req_list.back().req_num + 1;
    }
    this->req_list.push_back(req);
    return req.req_num;
}

/**
 * Get trigger which the application requests
 *
 * WMTrigger contains which application requests what role and where to put(area) and task.
 * This is used for input event to Window Policy Manager(state machine).
 *
 * @param     unsigned[in] request number
 * @param     bool[in,out] Check request number of the parameter is valid.
 * @return    WMTrigger which associates with the request number
 * @attention If the request number is not valid, parameter "found" is false
 *            and return value will be meaningless value.
 *            Caller can check the request parameter is valid.
 */
struct WMTrigger AppList::getRequest(unsigned req_num, bool *found)
{
    *found = false;
    for (const auto &x : this->req_list)
    {
        if (req_num == x.req_num)
        {
            *found = true;
            return x.trigger;
        }
    }
    HMI_SEQ_ERROR(req_num, "Couldn't get request : %d", req_num);
    return WMTrigger{"", "", "", Task::TASK_INVALID};
}

/**
 * Get actions which the application requests
 *
 * WMAciton contains the information of state transition.
 * In other words, it contains actions of Window Manager,
 * which role should be put to the area.
 *
 * @param     unsigned[in] request number
 * @param     bool[in,out] Check request number of the parameter is valid.
 * @return    WMTrigger which associates with the request number
 * @attention If the request number is not valid, parameter "found" is false
 *            and return value will be no reference pointer.
 *            Caller must check the request parameter is valid.
 */
const vector<struct WMAction> &AppList::getActions(unsigned req_num, bool* found)
{
    *found = false;
    for (auto &x : this->req_list)
    {
        if (req_num == x.req_num)
        {
            *found = true;
            return x.sync_draw_req;
        }
    }
    HMI_SEQ_ERROR(req_num, "Couldn't get action with the request : %d", req_num);
}

/**
 * Set actions to the request.
 *
 * Add actions to the request.
 * This function can be called many times, and actions increase.
 * This function is used for decision of actions of Window Manager
 * according to the result of Window Policy Manager.
 *
 * @param     unsigned[in] request number
 * @param     WMAction[in] Action of Window Manager.
 * @return    WMError If request number is not valid, FAIL will be returned.
 */
WMError AppList::setAction(unsigned req_num, const struct WMAction &action)
{
    std::lock_guard<std::mutex> lock(this->mtx);
    WMError result = WMError::FAIL;
    for (auto &x : this->req_list)
    {
        if (req_num != x.req_num)
        {
            continue;
        }
        x.sync_draw_req.push_back(action);
        result = WMError::SUCCESS;
        break;
    }
    return result;
}

/**
 * Note:
 * @note This function set action with parameters.
 *       If visible is true, it means app should be visible, so enddraw_finished parameter should be false.
 *       otherwise (visible is false) app should be invisible. Then enddraw_finished param is set to true.
 *       This function doesn't support actions for focus yet.
 */
/**
 * Set actions to the request.
 *
 * This function is overload function.
 * The feature is same as other one.
 *
 * @param     unsigned[in] request number
 * @param     string[in]   application id
 * @param     string[in]   role
 * @param     string[in]   area
 * @param     Task[in]     the role should be visible or not.
 * @return    WMError If request number is not valid, FAIL will be returned.
 * @attention This function set action with parameters, then caller doesn't need to create WMAction object.
 *            If visible is true, it means app should be visible, so enddraw_finished parameter will be false.
 *            otherwise (visible is false) app should be invisible. Then enddraw_finished param is set to true.
 *            This function doesn't support actions for focus yet.
 */
WMError AppList::setAction(unsigned req_num, const string &appid, const string &role, const string &area, TaskVisible visible)
{
    std::lock_guard<std::mutex> lock(this->mtx);
    WMError result = WMError::FAIL;
    for (auto &x : req_list)
    {
        if (req_num != x.req_num)
        {
            continue;
        }
        // If visible task is not invisible, redraw is required -> true
        bool edraw_f = (visible != TaskVisible::INVISIBLE) ? false : true;
        WMAction action{appid, role, area, visible, edraw_f};

        x.sync_draw_req.push_back(action);
        result = WMError::SUCCESS;
        break;
    }
    return result;
}

/**
 * Set end_draw_finished param is true
 *
 * This function checks
 *   - req_num is equal to current request number
 *   - appid and role are equeal to the appid and role stored in action list
 * If it is valid, set the action is finished.
 *
 * @param  unsigned[in] request number
 * @param  string[in]   application id
 * @param  string[in]   role
 * @return If the parameters are not valid in action list, returns false
 */
bool AppList::setEndDrawFinished(unsigned req_num, const string &appid, const string &role)
{
    std::lock_guard<std::mutex> lock(this->mtx);
    bool result = false;
    for (auto &x : req_list)
    {
        if (req_num < x.req_num)
        {
            break;
        }
        if (req_num == x.req_num)
        {
            for (auto &y : x.sync_draw_req)
            {
                if (y.appid == appid && y.role == role)
                {
                    HMI_SEQ_INFO(req_num, "Role %s finish redraw", y.role.c_str());
                    y.end_draw_finished = true;
                    result = true;
                }
            }
        }
    }
    this->reqDump();
    return result;
}

/**
 * Check all actions of the requested sequence is finished
 *
 * @param  unsigned[in] request_number
 * @return true if all action is set.
 */
bool AppList::endDrawFullfilled(unsigned req_num)
{
    bool result = false;
    for (const auto &x : req_list)
    {
        if (req_num < x.req_num)
        {
            break;
        }
        if (req_num == x.req_num)
        {
            result = true;
            for (const auto &y : x.sync_draw_req)
            {
                result &= y.end_draw_finished;
                if (!result)
                {
                    break;
                }
            }
        }
    }
    return result;
}

/**
 * Finish the request, then remove it.
 *
 * @param  unsigned[in] request_number
 * @return None
 * @note   Please call next after this function to receive or process next request.
 */
void AppList::removeRequest(unsigned req_num)
{
    std::lock_guard<std::mutex> lock(this->mtx);
    this->req_list.erase(remove_if(this->req_list.begin(), this->req_list.end(),
                                   [req_num](WMRequest x) {
                                       return x.req_num == req_num;
                                   }));
}

/**
 * Move the current request to next
 *
 * @param  None
 * @return None
 */
void AppList::next()
{
    std::lock_guard<std::mutex> lock(this->mtx);
    ++this->current_req;
    if (0 == this->current_req)
    {
        this->current_req = 1;
    }
}

/**
 * Check the request exists is in request list
 *
 * @param  None
 * @return true if WMRequest exists in the request list
 */
bool AppList::haveRequest() const
{
    return !this->req_list.empty();
}

void AppList::clientDump()
{
    DUMP("======= client dump =====");
    for (const auto &x : this->app2client)
    {
        const auto &y = x.second;
        y->dumpInfo();
    }
    DUMP("======= client dump end=====");
}

void AppList::reqDump()
{
    DUMP("======= req dump =====");
    DUMP("current request : %d", current_req);
    for (const auto &x : req_list)
    {
        DUMP("requested       : %d", x.req_num);
        DUMP("Trigger : (APPID :%s, ROLE :%s, AREA :%s, TASK: %d)",
             x.trigger.appid.c_str(),
             x.trigger.role.c_str(),
             x.trigger.area.c_str(),
             x.trigger.task);

        for (const auto &y : x.sync_draw_req)
        {
            DUMP(
                "Action  : (APPID :%s, ROLE :%s, AREA :%s, VISIBLE : %s, END_DRAW_FINISHED: %d)",
                y.appid.c_str(),
                y.role.c_str(),
                y.area.c_str(),
                (y.visible == TaskVisible::INVISIBLE) ? "invisible" : "visible",
                y.end_draw_finished);
        }
    }
    DUMP("======= req dump end =====");
}
} // namespace wm
