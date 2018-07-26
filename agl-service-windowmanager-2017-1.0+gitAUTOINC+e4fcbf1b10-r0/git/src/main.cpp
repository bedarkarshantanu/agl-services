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

#include <unistd.h>
#include <algorithm>
#include <mutex>
#include <json.h>
#include "../include/json.hpp"
#include "window_manager.hpp"
#include "json_helper.hpp"
#include "wayland_ivi_wm.hpp"

extern "C"
{
#include <afb/afb-binding.h>
#include <systemd/sd-event.h>
}

typedef struct WMClientCtxt
{
    std::string name;
    std::string role;
    WMClientCtxt(const char *appName, const char* appRole)
    {
        name = appName;
        role = appRole;
    }
} WMClientCtxt;

struct afb_instance
{
    std::unique_ptr<wl::display> display;
    wm::WindowManager wmgr;

    afb_instance() : display{new wl::display}, wmgr{this->display.get()} {}

    int init();
};

struct afb_instance *g_afb_instance;
std::mutex binding_m;

int afb_instance::init()
{
    return this->wmgr.init();
}

int display_event_callback(sd_event_source *evs, int /*fd*/, uint32_t events,
                           void * /*data*/)
{
    ST();

    if ((events & EPOLLHUP) != 0)
    {
        HMI_ERROR("wm", "The compositor hung up, dying now.");
        delete g_afb_instance;
        g_afb_instance = nullptr;
        goto error;
    }

    if ((events & EPOLLIN) != 0u)
    {
        {
            STN(display_read_events);
            g_afb_instance->wmgr.display->read_events();
            g_afb_instance->wmgr.set_pending_events();
        }
        {
            // We want do dispatch pending wayland events from within
            // the API context
            STN(winman_ping_api_call);
            afb_service_call("windowmanager", "ping", json_object_new_object(),
                            [](void *c, int st, json_object *j) {
                                STN(winman_ping_api_call_return);
                            },
                            nullptr);
        }
    }

    return 0;

error:
    sd_event_source_unref(evs);
    if (getenv("WINMAN_EXIT_ON_HANGUP") != nullptr)
    {
        exit(1);
    }
    return -1;
}

int _binding_init()
{
    HMI_NOTICE("wm", "WinMan ver. %s", WINMAN_VERSION_STRING);

    if (g_afb_instance != nullptr)
    {
        HMI_ERROR("wm", "Wayland context already initialized?");
        return 0;
    }

    if (getenv("XDG_RUNTIME_DIR") == nullptr)
    {
        HMI_ERROR("wm", "Environment variable XDG_RUNTIME_DIR not set");
        goto error;
    }

    {
        // wait until wayland compositor starts up.
        int cnt = 0;
        g_afb_instance = new afb_instance;
        while (!g_afb_instance->display->ok())
        {
            cnt++;
            if (20 <= cnt)
            {
                HMI_ERROR("wm", "Could not connect to compositor");
                goto error;
            }
            HMI_ERROR("wm", "Wait to start weston ...");
            sleep(1);
            delete g_afb_instance;
            g_afb_instance = new afb_instance;
        }
    }

    if (g_afb_instance->init() == -1)
    {
        HMI_ERROR("wm", "Could not connect to compositor");
        goto error;
    }

    {
        int ret = sd_event_add_io(afb_daemon_get_event_loop(), nullptr,
                                  g_afb_instance->display->get_fd(), EPOLLIN,
                                  display_event_callback, g_afb_instance);
        if (ret < 0)
        {
            HMI_ERROR("wm", "Could not initialize afb_instance event handler: %d", -ret);
            goto error;
        }
    }

    atexit([] { delete g_afb_instance; });

    return 0;

error:
    delete g_afb_instance;
    g_afb_instance = nullptr;
    return -1;
}

int binding_init() noexcept
{
    try
    {
        return _binding_init();
    }
    catch (std::exception &e)
    {
        HMI_ERROR("wm", "Uncaught exception in binding_init(): %s", e.what());
    }
    return -1;
}

static bool checkFirstReq(afb_req req)
{
    WMClientCtxt *ctxt = (WMClientCtxt *)afb_req_context_get(req);
    return (ctxt) ? false : true;
}

static void cbRemoveClientCtxt(void *data)
{
    WMClientCtxt *ctxt = (WMClientCtxt *)data;
    if (ctxt == nullptr)
    {
        return;
    }
    HMI_DEBUG("wm", "remove app %s", ctxt->name.c_str());
    // Lookup surfaceID and remove it because App is dead.
    auto pSid = g_afb_instance->wmgr.id_alloc.lookup(ctxt->role.c_str());
    if (pSid)
    {
        auto sid = *pSid;
        auto o_state = *g_afb_instance->wmgr.layers.get_layout_state(sid);
        if (o_state != nullptr)
        {
            if (o_state->main == sid)
            {
                o_state->main = -1;
            }
            else if (o_state->sub == sid)
            {
                o_state->sub = -1;
            }
        }
        g_afb_instance->wmgr.id_alloc.remove_id(sid);
        g_afb_instance->wmgr.layers.remove_surface(sid);
        g_afb_instance->wmgr.controller->sprops.erase(sid);
        g_afb_instance->wmgr.controller->surfaces.erase(sid);
        HMI_DEBUG("wm", "delete surfaceID %d", sid);
    }
    g_afb_instance->wmgr.removeClient(ctxt->name);
    delete ctxt;
}

void windowmanager_requestsurface(afb_req req) noexcept
{
    std::lock_guard<std::mutex> guard(binding_m);
#ifdef ST
    ST();
#endif
    if (g_afb_instance == nullptr)
    {
        afb_req_fail(req, "failed", "Binding not initialized, did the compositor die?");
        return;
    }

    try
    {
        const char *a_drawing_name = afb_req_value(req, "drawing_name");
        if (!a_drawing_name)
        {
            afb_req_fail(req, "failed", "Need char const* argument drawing_name");
            return;
        }

        /* Create Security Context */
        bool isFirstReq = checkFirstReq(req);
        if (!isFirstReq)
        {
            WMClientCtxt *ctxt = (WMClientCtxt *)afb_req_context_get(req);
            HMI_DEBUG("wm", "You're %s.", ctxt->name.c_str());
            if (ctxt->name != std::string(a_drawing_name))
            {
                afb_req_fail_f(req, "failed", "Dont request with other name: %s for now", a_drawing_name);
                HMI_DEBUG("wm", "Don't request with other name: %s for now", a_drawing_name);
                return;
            }
        }

        auto ret = g_afb_instance->wmgr.api_request_surface(
            afb_req_get_application_id(req), a_drawing_name);

        if (isFirstReq)
        {
            WMClientCtxt *ctxt = new WMClientCtxt(afb_req_get_application_id(req), a_drawing_name);
            HMI_DEBUG("wm", "create session for %s", ctxt->name.c_str());
            afb_req_session_set_LOA(req, 1);
            afb_req_context_set(req, ctxt, cbRemoveClientCtxt);
        }
        else
        {
            HMI_DEBUG("wm", "session already created for %s", a_drawing_name);
        }

        if (ret.is_err())
        {
            afb_req_fail(req, "failed", ret.unwrap_err());
            return;
        }

        afb_req_success(req, json_object_new_int(ret.unwrap()), "success");
    }
    catch (std::exception &e)
    {
        afb_req_fail_f(req, "failed", "Uncaught exception while calling requestsurface: %s", e.what());
        return;
    }
}

void windowmanager_requestsurfacexdg(afb_req req) noexcept
{
    std::lock_guard<std::mutex> guard(binding_m);
#ifdef ST
    ST();
#endif
    if (g_afb_instance == nullptr)
    {
        afb_req_fail(req, "failed", "Binding not initialized, did the compositor die?");
        return;
    }

    try
    {
        json_object *jreq = afb_req_json(req);

        json_object *j_drawing_name = nullptr;
        if (!json_object_object_get_ex(jreq, "drawing_name", &j_drawing_name))
        {
            afb_req_fail(req, "failed", "Need char const* argument drawing_name");
            return;
        }
        char const *a_drawing_name = json_object_get_string(j_drawing_name);

        json_object *j_ivi_id = nullptr;
        if (!json_object_object_get_ex(jreq, "ivi_id", &j_ivi_id))
        {
            afb_req_fail(req, "failed", "Need char const* argument ivi_id");
            return;
        }
        char const *a_ivi_id = json_object_get_string(j_ivi_id);

        auto ret = g_afb_instance->wmgr.api_request_surface(
            afb_req_get_application_id(req), a_drawing_name, a_ivi_id);
        if (ret != nullptr)
        {
            afb_req_fail(req, "failed", ret);
            return;
        }

        afb_req_success(req, NULL, "success");
    }
    catch (std::exception &e)
    {
        afb_req_fail_f(req, "failed", "Uncaught exception while calling requestsurfacexdg: %s", e.what());
        return;
    }
}

void windowmanager_activatewindow(afb_req req) noexcept
{
    std::lock_guard<std::mutex> guard(binding_m);
#ifdef ST
    ST();
#endif
    if (g_afb_instance == nullptr)
    {
        afb_req_fail(req, "failed", "Binding not initialized, did the compositor die?");
        return;
    }

    try
    {
        const char *a_drawing_name = afb_req_value(req, "drawing_name");
        if (!a_drawing_name)
        {
            afb_req_fail(req, "failed", "Need char const* argument drawing_name");
            return;
        }

        const char *a_drawing_area = afb_req_value(req, "drawing_area");
        if (!a_drawing_area)
        {
            afb_req_fail(req, "failed", "Need char const* argument drawing_area");
            return;
        }

        g_afb_instance->wmgr.api_activate_surface(
            afb_req_get_application_id(req),
            a_drawing_name, a_drawing_area,
            [&req](const char *errmsg) {
                if (errmsg != nullptr)
                {
                    HMI_ERROR("wm", errmsg);
                    afb_req_fail(req, "failed", errmsg);
                    return;
                }
                afb_req_success(req, NULL, "success");
            });
    }
    catch (std::exception &e)
    {
        HMI_WARNING("wm", "failed: Uncaught exception while calling activatesurface: %s", e.what());
        g_afb_instance->wmgr.exceptionProcessForTransition();
        return;
    }
}

void windowmanager_deactivatewindow(afb_req req) noexcept
{
    std::lock_guard<std::mutex> guard(binding_m);
#ifdef ST
    ST();
#endif
    if (g_afb_instance == nullptr)
    {
        afb_req_fail(req, "failed", "Binding not initialized, did the compositor die?");
        return;
    }

    try
    {
        const char *a_drawing_name = afb_req_value(req, "drawing_name");
        if (!a_drawing_name)
        {
            afb_req_fail(req, "failed", "Need char const* argument drawing_name");
            return;
        }

        g_afb_instance->wmgr.api_deactivate_surface(
            afb_req_get_application_id(req), a_drawing_name,
            [&req](const char *errmsg) {
                if (errmsg != nullptr)
                {
                    HMI_ERROR("wm", errmsg);
                    afb_req_fail(req, "failed", errmsg);
                    return;
                }
                afb_req_success(req, NULL, "success");
            });
    }
    catch (std::exception &e)
    {
        HMI_WARNING("wm", "failed: Uncaught exception while calling deactivatesurface: %s", e.what());
        g_afb_instance->wmgr.exceptionProcessForTransition();
        return;
    }
}

void windowmanager_enddraw(afb_req req) noexcept
{
    std::lock_guard<std::mutex> guard(binding_m);
#ifdef ST
    ST();
#endif
    if (g_afb_instance == nullptr)
    {
        afb_req_fail(req, "failed", "Binding not initialized, did the compositor die?");
        return;
    }

    try
    {
        const char *a_drawing_name = afb_req_value(req, "drawing_name");
        if (!a_drawing_name)
        {
            afb_req_fail(req, "failed", "Need char const* argument drawing_name");
            return;
        }
        afb_req_success(req, NULL, "success");

        g_afb_instance->wmgr.api_enddraw(
            afb_req_get_application_id(req), a_drawing_name);
    }
    catch (std::exception &e)
    {
        HMI_WARNING("wm", "failed: Uncaught exception while calling enddraw: %s", e.what());
        g_afb_instance->wmgr.exceptionProcessForTransition();
        return;
    }
}

void windowmanager_getdisplayinfo_thunk(afb_req req) noexcept
{
    std::lock_guard<std::mutex> guard(binding_m);
#ifdef ST
    ST();
#endif
    if (g_afb_instance == nullptr)
    {
        afb_req_fail(req, "failed", "Binding not initialized, did the compositor die?");
        return;
    }

    try
    {
        auto ret = g_afb_instance->wmgr.api_get_display_info();
        if (ret.is_err())
        {
            afb_req_fail(req, "failed", ret.unwrap_err());
            return;
        }

        afb_req_success(req, ret.unwrap(), "success");
    }
    catch (std::exception &e)
    {
        afb_req_fail_f(req, "failed", "Uncaught exception while calling getdisplayinfo: %s", e.what());
        return;
    }
}

void windowmanager_getareainfo_thunk(afb_req req) noexcept
{
    std::lock_guard<std::mutex> guard(binding_m);
#ifdef ST
    ST();
#endif
    if (g_afb_instance == nullptr)
    {
        afb_req_fail(req, "failed", "Binding not initialized, did the compositor die?");
        return;
    }

    try
    {
        json_object *jreq = afb_req_json(req);

        json_object *j_drawing_name = nullptr;
        if (!json_object_object_get_ex(jreq, "drawing_name", &j_drawing_name))
        {
            afb_req_fail(req, "failed", "Need char const* argument drawing_name");
            return;
        }
        char const *a_drawing_name = json_object_get_string(j_drawing_name);

        auto ret = g_afb_instance->wmgr.api_get_area_info(a_drawing_name);
        if (ret.is_err())
        {
            afb_req_fail(req, "failed", ret.unwrap_err());
            return;
        }

        afb_req_success(req, ret.unwrap(), "success");
    }
    catch (std::exception &e)
    {
        afb_req_fail_f(req, "failed", "Uncaught exception while calling getareainfo: %s", e.what());
        return;
    }
}

void windowmanager_wm_subscribe(afb_req req) noexcept
{
    std::lock_guard<std::mutex> guard(binding_m);
#ifdef ST
    ST();
#endif
    if (g_afb_instance == nullptr)
    {
        afb_req_fail(req, "failed", "Binding not initialized, did the compositor die?");
        return;
    }

    try
    {
        json_object *jreq = afb_req_json(req);
        json_object *j = nullptr;
        if (!json_object_object_get_ex(jreq, "event", &j))
        {
            afb_req_fail(req, "failed", "Need char const* argument event");
            return;
        }
        int event_type = json_object_get_int(j);
        const char *event_name = g_afb_instance->wmgr.kListEventName[event_type];
        struct afb_event event = g_afb_instance->wmgr.map_afb_event[event_name];
        int ret = afb_req_subscribe(req, event);
        if (ret)
        {
            afb_req_fail(req, "failed", "Error: afb_req_subscribe()");
            return;
        }
        afb_req_success(req, NULL, "success");
    }
    catch (std::exception &e)
    {
        afb_req_fail_f(req, "failed", "Uncaught exception while calling wm_subscribe: %s", e.what());
        return;
    }
}

void windowmanager_list_drawing_names(afb_req req) noexcept
{
    std::lock_guard<std::mutex> guard(binding_m);
#ifdef ST
    ST();
#endif
    if (g_afb_instance == nullptr)
    {
        afb_req_fail(req, "failed", "Binding not initialized, did the compositor die?");
        return;
    }

    try
    {

        nlohmann::json j = g_afb_instance->wmgr.id_alloc.name2id;
        auto ret = wm::Ok(json_tokener_parse(j.dump().c_str()));
        if (ret.is_err())
        {
            afb_req_fail(req, "failed", ret.unwrap_err());
            return;
        }

        afb_req_success(req, ret.unwrap(), "success");
    }
    catch (std::exception &e)
    {
        afb_req_fail_f(req, "failed", "Uncaught exception while calling list_drawing_names: %s", e.what());
        return;
    }
}

void windowmanager_ping(afb_req req) noexcept
{
    std::lock_guard<std::mutex> guard(binding_m);
#ifdef ST
    ST();
#endif
    if (g_afb_instance == nullptr)
    {
        afb_req_fail(req, "failed", "Binding not initialized, did the compositor die?");
        return;
    }

    try
    {

        g_afb_instance->wmgr.api_ping();

        afb_req_success(req, NULL, "success");
    }
    catch (std::exception &e)
    {
        afb_req_fail_f(req, "failed", "Uncaught exception while calling ping: %s", e.what());
        return;
    }
}

void windowmanager_debug_status(afb_req req) noexcept
{
    std::lock_guard<std::mutex> guard(binding_m);
#ifdef ST
    ST();
#endif
    if (g_afb_instance == nullptr)
    {
        afb_req_fail(req, "failed", "Binding not initialized, did the compositor die?");
        return;
    }

    try
    {

        json_object *jr = json_object_new_object();
        json_object_object_add(jr, "surfaces",
                               to_json(g_afb_instance->wmgr.controller->sprops));
        json_object_object_add(jr, "layers", to_json(g_afb_instance->wmgr.controller->lprops));

        afb_req_success(req, jr, "success");
    }
    catch (std::exception &e)
    {
        afb_req_fail_f(req, "failed", "Uncaught exception while calling debug_status: %s", e.what());
        return;
    }
}

void windowmanager_debug_layers(afb_req req) noexcept
{
    std::lock_guard<std::mutex> guard(binding_m);
#ifdef ST
    ST();
#endif
    if (g_afb_instance == nullptr)
    {
        afb_req_fail(req, "failed", "Binding not initialized, did the compositor die?");
        return;
    }

    try
    {
        auto ret = wm::Ok(json_tokener_parse(g_afb_instance->wmgr.layers.to_json().dump().c_str()));

        afb_req_success(req, ret, "success");
    }
    catch (std::exception &e)
    {
        afb_req_fail_f(req, "failed", "Uncaught exception while calling debug_layers: %s", e.what());
        return;
    }
}

void windowmanager_debug_surfaces(afb_req req) noexcept
{
    std::lock_guard<std::mutex> guard(binding_m);
#ifdef ST
    ST();
#endif
    if (g_afb_instance == nullptr)
    {
        afb_req_fail(req, "failed", "Binding not initialized, did the compositor die?");
        return;
    }

    try
    {

        auto ret = wm::Ok(to_json(g_afb_instance->wmgr.controller->sprops));
        if (ret.is_err())
        {
            afb_req_fail(req, "failed", ret.unwrap_err());
            return;
        }

        afb_req_success(req, ret.unwrap(), "success");
    }
    catch (std::exception &e)
    {
        afb_req_fail_f(req, "failed", "Uncaught exception while calling debug_surfaces: %s", e.what());
        return;
    }
}

void windowmanager_debug_terminate(afb_req req) noexcept
{
    std::lock_guard<std::mutex> guard(binding_m);
#ifdef ST
    ST();
#endif
    if (g_afb_instance == nullptr)
    {
        afb_req_fail(req, "failed", "Binding not initialized, did the compositor die?");
        return;
    }

    try
    {

        if (getenv("WINMAN_DEBUG_TERMINATE") != nullptr)
        {
            raise(SIGKILL); // afb-daemon kills it's pgroup using TERM, which
                            // doesn't play well with perf
        }

        afb_req_success(req, NULL, "success");
    }
    catch (std::exception &e)
    {
        afb_req_fail_f(req, "failed", "Uncaught exception while calling debug_terminate: %s", e.what());
        return;
    }
}

const struct afb_verb_v2 windowmanager_verbs[] = {
    {"requestsurface", windowmanager_requestsurface, nullptr, nullptr, AFB_SESSION_NONE},
    {"requestsurfacexdg", windowmanager_requestsurfacexdg, nullptr, nullptr, AFB_SESSION_NONE},
    {"activatewindow", windowmanager_activatewindow, nullptr, nullptr, AFB_SESSION_NONE},
    {"deactivatewindow", windowmanager_deactivatewindow, nullptr, nullptr, AFB_SESSION_NONE},
    {"enddraw", windowmanager_enddraw, nullptr, nullptr, AFB_SESSION_NONE},
    {"getdisplayinfo", windowmanager_getdisplayinfo_thunk, nullptr, nullptr, AFB_SESSION_NONE},
    {"getareainfo", windowmanager_getareainfo_thunk, nullptr, nullptr, AFB_SESSION_NONE},
    {"wm_subscribe", windowmanager_wm_subscribe, nullptr, nullptr, AFB_SESSION_NONE},
    {"list_drawing_names", windowmanager_list_drawing_names, nullptr, nullptr, AFB_SESSION_NONE},
    {"ping", windowmanager_ping, nullptr, nullptr, AFB_SESSION_NONE},
    {"debug_status", windowmanager_debug_status, nullptr, nullptr, AFB_SESSION_NONE},
    {"debug_layers", windowmanager_debug_layers, nullptr, nullptr, AFB_SESSION_NONE},
    {"debug_surfaces", windowmanager_debug_surfaces, nullptr, nullptr, AFB_SESSION_NONE},
    {"debug_terminate", windowmanager_debug_terminate, nullptr, nullptr, AFB_SESSION_NONE},
    {}};

extern "C" const struct afb_binding_v2 afbBindingV2 = {
    "windowmanager", nullptr, nullptr, windowmanager_verbs, nullptr, binding_init, nullptr, 0};
