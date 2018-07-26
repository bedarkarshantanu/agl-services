#pragma once

/*
 * Copyright (C) 2018 "IoT.bzh"
 * Author Loïc Collignon <loic.collignon@iot.bzh>
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

#include <exception>
#include <functional>
#include <sstream>
#include <vector>
#include <list>
#include <map>
#include <cassert>

#include "config_entry.hpp"
#include "role.hpp"

#define HL_API_NAME "ahl-4a"
#define HL_API_INFO "Audio high level API for AGL applications"
#define HAL_MGR_API "4a-hal-manager"

#include "afb-binding-common.h"

class ahl_binding_t
{
    using role_action = std::function<void(afb_request*, std::string, std::string, json_object*)>;

private:
    afb_dynapi* handle_;
    std::vector<role_t> roles_;

    explicit ahl_binding_t();

    void load_static_verbs();


    void load_controller_configs();
    int load_controller_config(const std::string& path);
    int update_streams();
    void update_stream(std::string hal, std::string stream, std::string deviceuri);
    int create_api_verb(role_t* r);

    void policy_open(afb_request* req, const role_t& role);

public:
    static ahl_binding_t& instance();
    int preinit(afb_dynapi* handle);
    int init();
    void event(std::string name, json_object* arg);
    void get_roles(afb_request* req);

    const std::vector<role_t> roles() const;
    afb_dynapi* handle() const;

    void audiorole(afb_request* req);
    int parse_roles_config(json_object* o);
};


