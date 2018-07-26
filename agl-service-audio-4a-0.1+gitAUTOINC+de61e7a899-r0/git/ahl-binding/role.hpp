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

#include <vector>
#include "interrupt.hpp"
#include "afb-binding-common.h"

class role_t
{
private:
    // Members filled by config
    std::string uid_;
    std::string description_;
    std::string hal_;
    std::string stream_;
    int priority_;
    std::vector<interrupt_t> interrupts_;

    std::string device_uri_;
    bool opened_;
    
    int apply_policy(afb_request* req);

    void do_mute(afb_request*, bool);

public:
    explicit role_t() = default;
    explicit role_t(const role_t&) = default;
    explicit role_t(role_t&&) = default;

    ~role_t() = default;
    
    role_t& operator=(const role_t&) = default;
    role_t& operator=(role_t&&) = default;

    static role_t from_json(json_object* o);

    explicit role_t(json_object* j);
    
    role_t& operator<<(json_object* j);

    std::string uid() const;
    std::string description() const;
    std::string hal() const;
    std::string stream() const;
    int priority() const;
    const std::vector<interrupt_t>& interrupts() const;
    std::string device_uri() const;
    bool opened() const;
    
    void uid(std::string v);
    void description(std::string v);
    void hal(std::string v);
    void stream(std::string v);
    void device_uri(std::string v);
    void priority(int v);

    void invoke(afb_request* r);

    void open(afb_request* r, json_object* o);
    void close(afb_request* r, json_object* o);
    void volume(afb_request* r, json_object* o);
    void interrupt(afb_request* r, json_object* o);
    void mute(afb_request* r, json_object* o);
    void unmute(afb_request* r, json_object* o);
};
