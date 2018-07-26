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

#ifndef WINDOWMANAGER_CLIENT_HPP
#define WINDOWMANAGER_CLIENT_HPP

#include <vector>
#include <string>
#include <unordered_map>

extern "C"
{
#define AFB_BINDING_VERSION 2
#include <afb/afb-binding.h>
}

namespace wm
{

enum WM_CLIENT_ERROR_EVENT
{
    UNKNOWN_ERROR
};

class WMClient
{
  public:
    WMClient();
    WMClient(const std::string &appid, unsigned layer,
            unsigned surface, const std::string &role);
    WMClient(const std::string &appid, const std::string &role);
    virtual ~WMClient();

    std::string appID() const;
    unsigned surfaceID(const std::string &role) const;
    unsigned layerID() const;
    std::string role(unsigned surface) const;
    void registerLayer(unsigned layer);
    bool addSurface(const std::string& role, unsigned surface);
    bool removeSurfaceIfExist(unsigned surface);
    bool removeRole(const std::string& role);

#ifndef GTEST_ENABLED
    bool subscribe(afb_req req, const std::string &event_name);
    void emitError(WM_CLIENT_ERROR_EVENT ev);
#endif

    void dumpInfo();

  private:
    std::string id;
    unsigned layer;
    std::unordered_map<std::string, unsigned> role2surface;
#if GTEST_ENABLED
    // This is for unit test. afb_make_event occurs sig11 if call not in afb-binding
    std::unordered_map<std::string, std::string> event2list;
#else
    std::unordered_map<std::string, struct afb_event> event2list;
#endif
};
} // namespace wm

#endif