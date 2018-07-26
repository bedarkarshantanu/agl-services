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

#ifndef ALLOCATE_LIST_HPP
#define ALLOCATE_LIST_HPP
#include <vector>
#include <string>
#include <map>
#include <memory>
#include <mutex>
#include "wm_client.hpp"
#include "request.hpp"
#include "wm_error.hpp"

namespace wm
{

/* using std::experimental::nullopt;
using std::experimental::optional; */

class AppList
{
  public:
    AppList();
    virtual ~AppList();
    AppList(const AppList &obj) = delete;

    // Client Database Interface
    /* TODO: consider, which is better WMClient as parameter or not
       If the WMClient should be more flexible, I think this param should be WMClient class
    */
    void addClient(const std::string &appid, unsigned layer,
                    unsigned surface,const std::string &role);
    void removeClient(const std::string &appid);
    bool contains(const std::string &appid) const;
    int  countClient() const;
    std::shared_ptr<WMClient> lookUpClient(const std::string &appid);
    void removeSurface(unsigned surface);
    std::string getAppID(unsigned surface, const std::string &role, bool *found) const;

    // Request Interface
    unsigned currentRequestNumber() const;
    unsigned getRequestNumber(const std::string &appid) const;
    unsigned addRequest(WMRequest req);
    WMError setAction(unsigned req_num, const struct WMAction &action);
    WMError setAction(unsigned req_num, const std::string &appid,
                    const std::string &role, const std::string &area, TaskVisible visible);
    bool setEndDrawFinished(unsigned req_num, const std::string &appid, const std::string &role);
    bool endDrawFullfilled(unsigned req_num);
    void removeRequest(unsigned req_num);
    void next();
    bool haveRequest() const;

    struct WMTrigger getRequest(unsigned req_num, bool* found);
    const std::vector<struct WMAction> &getActions(unsigned req_num, bool* found);

    void clientDump();
    void reqDump();

  private:
    std::vector<WMRequest> req_list;
    std::unordered_map<std::string, std::shared_ptr<WMClient>> app2client;
    unsigned current_req;
    std::mutex mtx;
};

} // namespace wm
#endif // ALLOCATE_LIST_HPP