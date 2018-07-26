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

#include <algorithm>
#include "ahl-binding.hpp"

afb_dynapi* AFB_default; // BUG: Is it possible to get rid of this ?

/**
 * @brief Callback invoked on new api creation.
 * @param[in] handle Handle to the new api.
 * @return Status code, zero if success.
 */
int ahl_api_create(void*, struct afb_dynapi* handle)
{
    return ahl_binding_t::instance().preinit(handle);
}

/**
 * @brief Entry point for dynamic API.
 * @param[in] handle Handle to start with for API creation.
 * @return Status code, zero if success.
 */
int afbBindingEntry(afb_dynapi* handle)
{
    using namespace std::placeholders;
    assert(handle != nullptr);

    AFB_default = handle;

    return afb_dynapi_new_api(
        handle,
        HL_API_NAME,
        HL_API_INFO,
        1,
        ahl_api_create,
        nullptr
    );
}

/**
 * @brief Callback invoked when API enter the init phase.
 * @return Status code, zero if success.
 */
int ahl_api_init(afb_dynapi*)
{
    return ahl_binding_t::instance().init();
}

/**
 * @brief Callback invoked when an event is received.
 * @param[in] e Event's name.
 * @param[in] o Event's args.
 */
void ahl_api_on_event(afb_dynapi*, const char* e, struct json_object* o)
{
    ahl_binding_t::instance().event(e, o);
}

/**
 * @brief Callback invoked when a 'roles' section is found in config file.
 * @param[in] o Config section to handle.
 * @return Status code, zero if success.
 */
int ahl_api_config_roles(afb_dynapi*, CtlSectionT*, json_object* o)
{
    return ahl_binding_t::instance().parse_roles_config(o);
}

/**
 * @brief Callback invoked when clients call the verb 'get_roles'.
 * @param[in] req Request to handle.
 */
void ahl_api_get_roles(afb_request* req)
{
    ahl_binding_t::instance().get_roles(req);
}

/**
 * @brief Callback invoked when clients call a 'role' verb.
 * @param[in] req Request to handle.
 * 
 * Handle dynamic verbs based on role name ('multimedia', 'navigation', ...)
 */
void ahl_api_role(afb_request* req)
{
    role_t* role = (role_t*)req->vcbdata;
    assert(role != nullptr);

    role->invoke(req);
}

/**
 * @brief Default constructor.
 */
ahl_binding_t::ahl_binding_t()
    : handle_{nullptr}
{
}

/**
 * @brief Get the singleton instance.
 * @return The unique instance.
 */
ahl_binding_t& ahl_binding_t::instance()
{
    static ahl_binding_t s;
    return s;
}

/**
 * @brief This method is called during the pre-init phase of loading the binding.
 * @param[in] handle Handle to the api.
 * @return Status code, zero if success.
 */
int ahl_binding_t::preinit(afb_dynapi* handle)
{
    handle_ = handle;

    try
    {
        load_static_verbs();
        load_controller_configs();

        if (afb_dynapi_on_event(handle_, ahl_api_on_event))
            throw std::runtime_error("Failed to register event handler callback.");

        if (afb_dynapi_on_init(handle_, ahl_api_init))
            throw std::runtime_error("Failed to register init handler callback.");
    }
    catch(std::exception& e)
    {
        AFB_DYNAPI_ERROR(handle, "%s", e.what());
        return -1;
    }

    return 0;
}

/**
 * @brief Initialize the API.
 */
int ahl_binding_t::init()
{
    using namespace std::placeholders;
    
    if (afb_dynapi_require_api(handle_, HAL_MGR_API, 1))
    {
        AFB_DYNAPI_ERROR(handle_, "Failed to require '%s' API!", HAL_MGR_API);
        return -1;
    }
    AFB_DYNAPI_NOTICE(handle_, "Required '%s' API found!", HAL_MGR_API);
    
    if (afb_dynapi_require_api(handle_, "smixer", 1))
    {
        AFB_DYNAPI_ERROR(handle_, "Failed to require 'smixer' API!");
        return -1;
    }
    AFB_DYNAPI_NOTICE(handle_, "Required 'smixer' API found!");

    afb_dynapi_seal(handle_);
    AFB_DYNAPI_NOTICE(handle_, "API is now sealed!");

    if (update_streams()) return -1; 
    return 0;
}

/**
 * @brief Update audio roles definition by binding to streams.
 * @return Status code, zero if success.
 */
int ahl_binding_t::update_streams()
{
    json_object* loaded = nullptr;
    json_object* response = nullptr;
    size_t hals_count = 0, streams_count = 0;

    if (afb_dynapi_call_sync(handle_, "4a-hal-manager", "loaded", json_object_new_object(), &loaded))
    {
        AFB_DYNAPI_ERROR(handle_, "Failed to call 'loaded' verb on '4a-hal-manager' API!");
        if (loaded) AFB_DYNAPI_NOTICE(handle_, "%s", json_object_to_json_string(loaded));
        return -1;
    }
    json_bool ret = json_object_object_get_ex(loaded, "response", &response);
    if (!ret)
    {
        AFB_DYNAPI_ERROR(handle_, "Maformed response; missing 'response' field");
        return -1;
    }
    hals_count = json_object_array_length(response);

    for(int i = 0; i < hals_count; ++i)
    {
        json_object* info = nullptr;

        const char* halname = json_object_get_string(json_object_array_get_idx(response, i));
        AFB_DYNAPI_DEBUG(handle_, "Found an active HAL: %s", halname);

        if (afb_dynapi_call_sync(handle_, halname, "info", json_object_new_object(), &info))
        {
            AFB_DYNAPI_ERROR(handle_, "Failed to call 'info' verb on '%s' API!", halname);
            if (info) AFB_DYNAPI_NOTICE(handle_, "%s", json_object_to_json_string(info));
            return -1;
        }

        json_object * responseJ = nullptr;
        json_object_object_get_ex(info, "response", &responseJ);

        json_object* streamsJ = nullptr;
        json_object_object_get_ex(responseJ, "streams", &streamsJ);
        streams_count = json_object_array_length(streamsJ);
        for(int j = 0; j < streams_count; ++j)
        {
            json_object * nameJ = nullptr, * cardIdJ = nullptr;
            json_object * streamJ = json_object_array_get_idx(streamsJ, j);

            json_object_object_get_ex(streamJ, "name", &nameJ);
            json_object_object_get_ex(streamJ, "cardId", &cardIdJ);

            update_stream(
                halname,
                json_object_get_string(nameJ),
                json_object_get_string(cardIdJ)
            );
        }

        json_object_put(info);        
    }
    json_object_put(loaded);

    return 0;
}

/**
 * @brief Update the stream info for audio roles.
 * @param[in] halname The hal on which the stream is.
 * @param[in] stream The name of the stream.
 * @param[in] deviceid The device ID to return when opening an audio role.
 */
void ahl_binding_t::update_stream(std::string halname, std::string stream, std::string deviceid)
{
    for(auto& r : roles_)
    {
        if(r.stream() == stream)
        {
            if (r.device_uri().size())
                AFB_DYNAPI_WARNING(handle_, "Multiple stream with same name: '%s'.", stream.c_str());
            else
            {
                r.device_uri(deviceid);
                r.hal(halname);
            }
        }
    }
}

void ahl_binding_t::event(std::string name, json_object* arg)
{
    AFB_DYNAPI_DEBUG(handle_, "Event '%s' received with the following arg: %s", name.c_str(), json_object_to_json_string(arg));
}

void ahl_binding_t::load_static_verbs()
{
    if (afb_dynapi_add_verb(
            handle_,
            "get_roles",
            "Retrieve array of available audio roles",
            ahl_api_get_roles,
            nullptr,
            nullptr,
            AFB_SESSION_NONE_X2))
    {
        throw std::runtime_error("Failed to add 'get_role' verb to the API.");
    }
}

void ahl_binding_t::load_controller_configs()
{
    char* dir_list = getenv("CONTROL_CONFIG_PATH");
    if (!dir_list) dir_list = strdup(CONTROL_CONFIG_PATH);
    struct json_object* config_files = CtlConfigScan(dir_list, "policy");
    if (!config_files) throw std::runtime_error("No config files found!");

    // Only one file should be found this way, but read all just in case
    size_t config_files_count = json_object_array_length(config_files);
    for(int i = 0; i < config_files_count; ++i)
    {
        config_entry_t file {json_object_array_get_idx(config_files, i)};

        if(load_controller_config(file.filepath()) < 0)
        {
            std::stringstream ss;
            ss  << "Failed to load config file '"
                << file.filename()
                << "' from '"
                << file.fullpath()
                << "'!";
            throw std::runtime_error(ss.str());
        }
    }
}

int ahl_binding_t::load_controller_config(const std::string& path)
{
    CtlConfigT* controller_config;

    controller_config = CtlLoadMetaData(handle_, path.c_str());
    if (!controller_config)
    {
        AFB_DYNAPI_ERROR(handle_, "Failed to load controller from config file!");
        return -1;
    }
    
    static CtlSectionT controller_sections[] =
    {
        {.key = "plugins",  .uid = nullptr, .info = nullptr, .loadCB = PluginConfig,  .handle = nullptr, .actions = nullptr},
        {.key = "onload",   .uid = nullptr, .info = nullptr, .loadCB = OnloadConfig,  .handle = nullptr, .actions = nullptr},
        {.key = "controls", .uid = nullptr, .info = nullptr, .loadCB = ControlConfig, .handle = nullptr, .actions = nullptr},
        {.key = "events",   .uid = nullptr, .info = nullptr, .loadCB = EventConfig,   .handle = nullptr, .actions = nullptr},
        {.key = "roles",    .uid = nullptr, .info = nullptr, .loadCB = ahl_api_config_roles, .handle = nullptr, .actions = nullptr },
        {.key = nullptr,    .uid = nullptr, .info = nullptr, .loadCB = nullptr,       .handle = nullptr, .actions = nullptr}
    };

    CtlLoadSections(handle_, controller_config, controller_sections);

    return 0;
}

int ahl_binding_t::parse_roles_config(json_object* o)
{
    assert(o != nullptr);
    assert(json_object_is_type(o, json_type_array));

    if (roles_.size()) return 0; // Roles already added, ignore.
    
    size_t count = json_object_array_length(o);
    roles_.reserve(count);
    for(int i = 0; i < count; ++i)
    {
        json_object* jr = json_object_array_get_idx(o, i);
        assert(jr != nullptr);

        roles_.push_back(role_t(jr));
        role_t& r = roles_[roles_.size() - 1];
        if(create_api_verb(&r))
            return -1;
    }

    return 0;
}

int ahl_binding_t::create_api_verb(role_t* r)
{
    AFB_DYNAPI_NOTICE(handle_, "New audio role: %s", r->uid().c_str());

     if (afb_dynapi_add_verb(
        handle_,
        r->uid().c_str(),
        r->description().c_str(),
        ahl_api_role,
        r,
        nullptr,
        AFB_SESSION_NONE_X2))
    {
        AFB_DYNAPI_ERROR(handle_, "Failed to add '%s' verb to the API.",
            r->uid().c_str());
        return -1;
    }

    return 0;
}

void ahl_binding_t::get_roles(afb_request* req)
{
    json_object* result = json_object_new_array();
    for(const auto& r : roles_)
        json_object_array_add(result, json_object_new_string(r.uid().c_str()));
    afb_request_success(req, result, nullptr);
}

const std::vector<role_t> ahl_binding_t::roles() const
{
    return roles_;
}

afb_dynapi* ahl_binding_t::handle() const
{
    return handle_;
}
