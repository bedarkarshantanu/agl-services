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

#ifndef WINDOW_MANAGER_HPP
#define WINDOW_MANAGER_HPP

#include <atomic>
#include <memory>
#include <unordered_map>
#include <experimental/optional>
#include "controller_hooks.hpp"
#include "layers.hpp"
#include "layout.hpp"
#include "wayland_ivi_wm.hpp"
#include "hmi-debug.h"
#include "request.hpp"
#include "wm_error.hpp"

struct json_object;

namespace wl
{
struct display;
}

namespace compositor
{
struct controller;
}

namespace wm
{

using std::experimental::optional;

/* DrawingArea name used by "{layout}.{area}" */
extern const char kNameLayoutNormal[];
extern const char kNameLayoutSplit[];
extern const char kNameAreaFull[];
extern const char kNameAreaMain[];
extern const char kNameAreaSub[];

/* Key for json obejct */
extern const char kKeyDrawingName[];
extern const char kKeyDrawingArea[];
extern const char kKeyDrawingRect[];
extern const char kKeyX[];
extern const char kKeyY[];
extern const char kKeyWidth[];
extern const char kKeyHeigh[];
extern const char kKeyWidthPixel[];
extern const char kKeyHeightPixel[];
extern const char kKeyWidthMm[];
extern const char kKeyHeightMm[];
extern const char kKeyIds[];

struct id_allocator
{
    unsigned next = 1;

    // Surfaces that where requested but not yet created
    std::unordered_map<unsigned, std::string> id2name;
    std::unordered_map<std::string, unsigned> name2id;

    id_allocator(id_allocator const &) = delete;
    id_allocator(id_allocator &&) = delete;
    id_allocator &operator=(id_allocator const &);
    id_allocator &operator=(id_allocator &&) = delete;

    // Insert and return a new ID
    unsigned generate_id(std::string const &name)
    {
        unsigned sid = this->next++;
        this->id2name[sid] = name;
        this->name2id[name] = sid;
        HMI_DEBUG("wm", "allocated new id %u with name %s", sid, name.c_str());
        return sid;
    }

    // Insert a new ID which defined outside
    void register_name_id(std::string const &name, unsigned sid)
    {
        this->id2name[sid] = name;
        this->name2id[name] = sid;
        HMI_DEBUG("wm", "register id %u with name %s", sid, name.c_str());
        return;
    }

    // Lookup by ID or by name
    optional<unsigned> lookup(std::string const &name) const
    {
        auto i = this->name2id.find(name);
        return i == this->name2id.end() ? nullopt : optional<unsigned>(i->second);
    }

    optional<std::string> lookup(unsigned id) const
    {
        auto i = this->id2name.find(id);
        return i == this->id2name.end() ? nullopt
                                        : optional<std::string>(i->second);
    }

    // Remove a surface id and name
    void remove_id(std::string const &name)
    {
        auto i = this->name2id.find(name);
        if (i != this->name2id.end())
        {
            this->id2name.erase(i->second);
            this->name2id.erase(i);
        }
    }

    void remove_id(unsigned id)
    {
        auto i = this->id2name.find(id);
        if (i != this->id2name.end())
        {
            this->name2id.erase(i->second);
            this->id2name.erase(i);
        }
    }
};

class WindowManager
{
  public:
    typedef std::unordered_map<uint32_t, struct compositor::rect> rect_map;
    typedef std::function<void(const char *err_msg)> reply_func;

    enum EventType
    {
        Event_Val_Min = 0,

        Event_Active = Event_Val_Min,
        Event_Inactive,

        Event_Visible,
        Event_Invisible,

        Event_SyncDraw,
        Event_FlushDraw,

        Event_ScreenUpdated,

        Event_Error,

        Event_Val_Max = Event_Error,
    };

    const std::vector<const char *> kListEventName{
        "active",
        "inactive",
        "visible",
        "invisible",
        "syncDraw",
        "flushDraw",
        "screenUpdated",
        "error"};

    struct controller_hooks chooks;

    // This is the one thing, we do not own.
    struct wl::display *display;

    std::unique_ptr<struct compositor::controller> controller;
    std::vector<std::unique_ptr<struct wl::output>> outputs;

    // track current layouts separately
    layer_map layers;

    // ID allocation and proxy methods for lookup
    struct id_allocator id_alloc;

    // Set by AFB API when wayland events need to be dispatched
    std::atomic<bool> pending_events;

    std::map<const char *, struct afb_event> map_afb_event;

    // Surface are info (x, y, w, h)
    rect_map area_info;

    // FOR CES DEMO
    std::vector<int> surface_bg;

    explicit WindowManager(wl::display *d);
    ~WindowManager() = default;

    WindowManager(WindowManager const &) = delete;
    WindowManager &operator=(WindowManager const &) = delete;
    WindowManager(WindowManager &&) = delete;
    WindowManager &operator=(WindowManager &&) = delete;

    int init();
    int dispatch_pending_events();
    void set_pending_events();

    result<int> api_request_surface(char const *appid, char const *drawing_name);
    char const *api_request_surface(char const *appid, char const *drawing_name, char const *ivi_id);
    void api_activate_surface(char const *appid, char const *drawing_name, char const *drawing_area, const reply_func &reply);
    void api_deactivate_surface(char const *appid, char const *drawing_name, const reply_func &reply);
    void api_enddraw(char const *appid, char const *drawing_name);
    result<json_object *> api_get_display_info();
    result<json_object *> api_get_area_info(char const *drawing_name);
    void api_ping();
    void send_event(char const *evname, char const *label);
    void send_event(char const *evname, char const *label, char const *area, int x, int y, int w, int h);

    // Events from the compositor we are interested in
    void surface_created(uint32_t surface_id);
    void surface_removed(uint32_t surface_id);

    void removeClient(const std::string &appid);
    void exceptionProcessForTransition();
    // Do not use this function
    void timerHandler();

  private:
    bool pop_pending_events();
    optional<int> lookup_id(char const *name);
    optional<std::string> lookup_name(int id);
    int init_layers();
    void surface_set_layout(int surface_id, const std::string& area = "");
    void layout_commit();

    // WM Events to clients
    void emit_activated(char const *label);
    void emit_deactivated(char const *label);
    void emit_syncdraw(char const *label, char const *area, int x, int y, int w, int h);
    void emit_syncdraw(const std::string &role, const std::string &area);
    void emit_flushdraw(char const *label);
    void emit_visible(char const *label, bool is_visible);
    void emit_invisible(char const *label);
    void emit_visible(char const *label);

    void activate(int id);
    void deactivate(int id);
    WMError setRequest(const std::string &appid, const std::string &role, const std::string &area,
                             Task task, unsigned *req_num);
    WMError doTransition(unsigned sequence_number);
    WMError checkPolicy(unsigned req_num);
    WMError startTransition(unsigned req_num);
    WMError setInvisibleTask(const std::string &role, bool split);

    WMError doEndDraw(unsigned req_num);
    WMError layoutChange(const WMAction &action);
    WMError visibilityChange(const WMAction &action);
    WMError setSurfaceSize(unsigned surface, const std::string& area);
    WMError changeCurrentState(unsigned req_num);
    void    emitScreenUpdated(unsigned req_num);

    void setTimer();
    void stopTimer();
    void processNextRequest();

    int loadOldRoleDb();
    const char* convertRoleOldToNew(char const *drawing_name);

    const char *check_surface_exist(const char *drawing_name);

    bool can_split(struct LayoutState const &state, int new_id);

  private:
    std::unordered_map<std::string, struct compositor::rect> area2size;
    std::unordered_map<std::string, std::string> roleold2new;
    std::unordered_map<std::string, std::string> rolenew2old;

    static const char* kDefaultOldRoleDb;
};

} // namespace wm

#endif // WINDOW_MANAGER_HPP
