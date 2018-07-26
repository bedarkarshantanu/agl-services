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

#include <json-c/json.h>
#include "wm_client.hpp"
#include "hmi-debug.h"

#define INVALID_SURFACE_ID 0

using std::string;
using std::vector;

namespace wm
{

static const vector<string> kWMEvents = {
    // Private event for applications
    "syncDraw", "flushDraw", "visible", "invisible", "active", "inactive", "error"};
static const vector<string> kErrorDescription = {
    "unknown-error"};

static const char kKeyDrawingName[] = "drawing_name";
static const char kKeyrole[] = "role";
static const char kKeyError[] = "error";
static const char kKeyErrorDesc[] = "kErrorDescription";

WMClient::WMClient(const string &appid, unsigned layer, unsigned surface, const string &role)
    : id(appid), layer(layer),
      role2surface(0)
{
    role2surface[role] = surface;
    for (auto x : kWMEvents)
    {
#if GTEST_ENABLED
        string ev = x;
#else
        afb_event ev = afb_daemon_make_event(x.c_str());
#endif
        event2list[x] = ev;
    }
}

WMClient::WMClient(const string &appid, const string &role)
    : id(appid),
      layer(0),
      role2surface(0),
      event2list(0)
{
    role2surface[role] = INVALID_SURFACE_ID;
    for (auto x : kWMEvents)
    {
#if GTEST_ENABLED
        string ev = x;
#else
        afb_event ev = afb_daemon_make_event(x.c_str());
#endif
        event2list[x] = ev;
    }
}

WMClient::~WMClient()
{
}

string WMClient::appID() const
{
    return this->id;
}

unsigned WMClient::surfaceID(const string &role) const
{
    if (0 == this->role2surface.count(role))
    {
        return INVALID_SURFACE_ID;
    }
    return this->role2surface.at(role);
}

std::string WMClient::role(unsigned surface) const
{
    for(const auto& x : this->role2surface)
    {
        if(x.second == surface)
        {
            return x.first;
        }
    }
    return std::string("");
}

unsigned WMClient::layerID() const
{
    return this->layer;
}

/**
 * Set layerID the client belongs to
 *
 * This function set layerID the client belongs to.
 * But this function may not used because the layer should be fixed at constructor.
 * So this function will be used to change layer by some reasons.
 *
 * @param     unsigned[in] layerID
 * @return    None
 * @attention WMClient can't have multiple layer
 */
void WMClient::registerLayer(unsigned layer)
{
    this->layer = layer;
}

/**
 * Add the pair of role and surface to the client
 *
 * This function set the pair of role and surface to the client.
 * This function is used for the client which has multi surfaces.
 * If the model and relationship for role and surface(layer)
 * is changed, this function will be changed
 * Current Window Manager doesn't use this function.
 *
 * @param     string[in] role
 * @param     unsigned[in] surface
 * @return    true
 */
bool WMClient::addSurface(const string &role, unsigned surface)
{
    HMI_DEBUG("wm", "Add role %s with surface %d", role.c_str(), surface);
    if (0 != this->role2surface.count(role))
    {
        HMI_NOTICE("wm", "override surfaceID %d with %d", this->role2surface[role], surface);
    }
    this->role2surface[role] = surface;
    return true;
}

bool WMClient::removeSurfaceIfExist(unsigned surface)
{
    bool ret = false;
    for (auto &x : this->role2surface)
    {
        if (surface == x.second)
        {
            HMI_INFO("wm", "Remove surface from client %s: role %s, surface: %d",
                                this->id.c_str(), x.first.c_str(), x.second);
            this->role2surface.erase(x.first);
            ret = true;
            break;
        }
    }
    return ret;
}

bool WMClient::removeRole(const string &role)
{
    bool ret = false;
    if (this->role2surface.count(role) != 0)
    {
        this->role2surface.erase(role);
        ret = true;
    }
    return ret;
}

#ifndef GTEST_ENABLED
bool WMClient::subscribe(afb_req req, const string &evname)
{
    if(evname != kKeyError){
        HMI_DEBUG("wm", "error is only enabeled for now");
        return false;
    }
    int ret = afb_req_subscribe(req, this->event2list[evname]);
    if (ret)
    {
        HMI_DEBUG("wm", "Failed to subscribe %s", evname.c_str());
        return false;
    }
    return true;
}

void WMClient::emitError(WM_CLIENT_ERROR_EVENT ev)
{
    if (!afb_event_is_valid(this->event2list[kKeyError])){
        HMI_ERROR("wm", "event err is not valid");
        return;
    }
    json_object *j = json_object_new_object();
    json_object_object_add(j, kKeyError, json_object_new_int(ev));
    json_object_object_add(j, kKeyErrorDesc, json_object_new_string(kErrorDescription[ev].c_str()));
    HMI_DEBUG("wm", "error: %d, description:%s", ev, kErrorDescription[ev].c_str());

    int ret = afb_event_push(this->event2list[kKeyError], j);
    if (ret != 0)
    {
        HMI_DEBUG("wm", "afb_event_push failed: %m");
    }
}
#endif

void WMClient::dumpInfo()
{
    DUMP("APPID : %s", id.c_str());
    DUMP("  LAYER : %d", layer);
    for (const auto &x : this->role2surface)
    {
        DUMP("  ROLE  : %s , SURFACE : %d", x.first.c_str(), x.second);
    }
}

} // namespace wm