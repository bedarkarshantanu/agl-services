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

#include "role.hpp"
#include "jsonc_utils.hpp"
#include "ahl-binding.hpp"

role_t::role_t(json_object* j)
{
    jcast(uid_, j, "uid");
    jcast(description_, j, "description");
    jcast(priority_, j, "priority");
    jcast(stream_, j, "stream");
    jcast_array(interrupts_, j, "interrupts");
    opened_ = false;
}

role_t& role_t::operator<<(json_object* j)
{
    jcast(uid_, j, "uid");
    jcast(description_, j, "description");
    jcast(priority_, j, "priority");
    jcast(stream_, j, "stream");
    jcast_array(interrupts_, j, "interrupts");
    return *this;
}

std::string role_t::uid() const
{
    return uid_;
}

std::string role_t::description() const
{
    return description_;
}

std::string role_t::hal() const
{
    return hal_;
}

std::string role_t::stream() const
{
    return stream_;
}

int role_t::priority() const
{
    return priority_;
}

std::string role_t::device_uri() const
{
    return device_uri_;
}

bool role_t::opened() const
{
    return opened_;
}

void role_t::uid(std::string v)
{
    uid_ = v;
}

void role_t::description(std::string v)
{
    description_ = v;
}

void role_t::hal(std::string v)
{
    hal_ = v;
}

void role_t::stream(std::string v)
{
    stream_ = v;
}

void role_t::priority(int v)
{
    priority_ = v;
}

void role_t::device_uri(std::string v)
{
    device_uri_ = v;
}

const std::vector<interrupt_t>& role_t::interrupts() const
{
    return interrupts_;
}

int role_t::apply_policy(afb_request* req)
{
    if(interrupts_.size())
    {
        const interrupt_t& i = interrupts_[0];
        /*if (i.type() == "mute")
        {
        }
        else if (i.type() == "continue")
        {
        }
        else if (i.type() == "cancel")
        {
        }
        else */if (i.type() == "ramp")
        {
            for(const auto& r: ahl_binding_t::instance().roles())
            {
                if (r.opened() && priority_ > r.priority())
                {
                    // { "ramp" : { "uid" : "ramp-slow", "volume" : 30 } }
                    json_object* arg = json_object_new_object();
                    json_object_object_add(arg, "ramp", i.args());
                    json_object_get(i.args());
                    json_object* result = nullptr;

                    AFB_DYNAPI_NOTICE(ahl_binding_t::instance().handle(),
                        "Call '%s'/'%s' '%s",
                        r.hal().c_str(), r.stream().c_str(), json_object_to_json_string(arg));

                    if(afb_dynapi_call_sync(ahl_binding_t::instance().handle(), r.hal().c_str(), r.stream().c_str(), arg, &result))
                    {
                        afb_request_fail(req, "Failed to call 'ramp' action on stream", nullptr);
                        return -1;
                    }
                    AFB_DYNAPI_NOTICE(ahl_binding_t::instance().handle(),
                        "POLICY: Applying a ramp to '%s' stream because '%s' is opened and have higher priority!",
                        r.stream().c_str(), stream_.c_str());
                }
            }
        }
        else
        {
            afb_request_fail(req, "Unkown interrupt uid!", nullptr);
            return -1;
        }
    }
    return 0;
}

void role_t::invoke(afb_request* req)
{
    json_object* arg = afb_request_json(req);
    if (arg == nullptr)
    {
        afb_request_fail(req, "No valid argument!", nullptr);
        return;
    }

    json_object* jaction;
    json_bool ret = json_object_object_get_ex(arg, "action", &jaction);
    if (!ret)
    {
        afb_request_fail(req, "No valid action!", nullptr);
        return;
    }

    std::string action = json_object_get_string(jaction);
    if (action.size() == 0)
    {
        afb_request_fail(req, "No valid action!", nullptr);
        return;
    }

         if (action == "open")      open(req, arg);
    else if (action == "close")     close(req, arg);
    else if (action == "volume")    volume(req, arg);
    else if (action == "interrupt") interrupt(req, arg);
    else if (action == "mute")      mute(req, arg);
    else if (action == "unmute")    unmute(req, arg);
    else afb_request_fail(req, "Unknown action!", nullptr);
}

void role_t::open(afb_request* r, json_object* o)
{
    if (opened_)
    {
        afb_request_fail(r, "Already opened!", nullptr);
        return;
    }

    if (!apply_policy(r))
    {
        afb_request_context_set(
            r,
            this,
            [](void* arg)
            {
                role_t * role = (role_t*) arg;
                afb_dynapi * api = ahl_binding_t::instance().handle();

                AFB_DYNAPI_NOTICE(api, "Released role %s\n", role->uid_.c_str());
                role->opened_ = false;

                // send a mute command to the HAL. We cannot reuse the do_mute function,
                // because in the context here, the afb_request is no longer valid.
                json_object* a = json_object_new_object();
                json_object_object_add(a, "mute", json_object_new_boolean(true));

                afb_dynapi_call(
                     api,
                     role->hal_.c_str(),
                     role->stream_.c_str(),
                     a,
                     NULL,
                     NULL);
            }
        );
        opened_ = true;

        json_object* result = json_object_new_object();
        json_object_object_add(result, "device_uri", json_object_new_string(device_uri_.c_str()));

        afb_request_success(r, result, nullptr);
    }
}

void role_t::close(afb_request* r, json_object* o)
{
    if (!opened_)
    {
        afb_request_success(r, nullptr, "Already closed!");
        return;
    }

    if(!afb_request_context_get(r))
    {
        afb_request_fail(r, "Stream is opened by another client!", nullptr);
        return;
    }

    opened_ = false;
    afb_request_success(r, nullptr, "Stream closed!");
}

void role_t::mute(afb_request * r, json_object* o) {
	do_mute(r, true);
}

void role_t::unmute(afb_request *r, json_object  *o) {
	do_mute(r, false);
}

void role_t::do_mute(afb_request * r, bool v) {

	json_object* a = json_object_new_object();
    json_object_object_add(a, "mute", json_object_new_boolean(v));
	afb_dynapi * api = ahl_binding_t::instance().handle();

    afb_dynapi_call(
        api,
        hal_.c_str(),
        stream_.c_str(),
        a,
        [](void* closure, int status, json_object* result, afb_dynapi* handle)
        {
            AFB_DYNAPI_DEBUG(handle, "Got the following answer: %s", json_object_to_json_string(result));
            afb_request* r = (afb_request*)closure;

            json_object_get(result);
            if (status) afb_request_fail(r, json_object_to_json_string(result), nullptr);
            else afb_request_success(r, result, nullptr);
            afb_request_unref(r);
        },
        afb_request_addref(r));
}

void role_t::volume(afb_request* r, json_object* o)
{
	afb_dynapi * api = ahl_binding_t::instance().handle();

    if(!afb_request_has_permission(r, "urn:AGL:permission::public:4a-audio-mixer"))
    {
        if (!opened_)
        {
            afb_request_fail(r, "You have to open the stream first!", nullptr);
            return;
        }

        if(!afb_request_context_get(r))
        {
            afb_request_fail(r, "Stream is opened by another client!", nullptr);
            return;
        }
    }
    else
    {
        AFB_DYNAPI_NOTICE(api, "Granted special audio-mixer permission to change volume");
    }

    json_object* value;
    json_bool ret = json_object_object_get_ex(o, "value", &value);
    if (!ret)
    {
        afb_request_fail(r, "No value given!", nullptr);
        return;
    }

    json_object_get(value);

    json_object* a = json_object_new_object();
    json_object_object_add(a, "volume", value);

    afb_dynapi_call(
        api,
        hal_.c_str(),
        stream_.c_str(),
        a,
        [](void* closure, int status, json_object* result, afb_dynapi* handle)
        {
            AFB_DYNAPI_DEBUG(handle, "Got the following answer: %s", json_object_to_json_string(result));
            afb_request* r = (afb_request*)closure;

            json_object_get(result);
            if (status) afb_request_fail(r, json_object_to_json_string(result), nullptr);
            else afb_request_success(r, result, nullptr);
            afb_request_unref(r);
        },
        afb_request_addref(r));
}

void role_t::interrupt(afb_request* r, json_object* o)
{
}
