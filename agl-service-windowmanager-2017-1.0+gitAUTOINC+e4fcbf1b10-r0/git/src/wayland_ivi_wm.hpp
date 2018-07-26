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

#ifndef WM_WAYLAND_HPP
#define WM_WAYLAND_HPP

#include "controller_hooks.hpp"
#include "ivi-wm-client-protocol.h"
#include "util.hpp"

#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

/**
 * @struct wayland_proxy
 */
template <typename ProxyT>
struct wayland_proxy
{
    std::unique_ptr<ProxyT, std::function<void(ProxyT *)>> proxy;
    wayland_proxy(wayland_proxy const &) = delete;
    wayland_proxy &operator=(wayland_proxy const &) = delete;
    wayland_proxy(void *p)
        : wayland_proxy(p,
                        reinterpret_cast<void (*)(ProxyT *)>(wl_proxy_destroy)) {}
    wayland_proxy(void *p, std::function<void(ProxyT *)> &&p_del)
        : proxy(std::unique_ptr<ProxyT, std::function<void(ProxyT *)>>(
              static_cast<ProxyT *>(p), p_del)) {}
};

/**
 * namespace wl
 */
namespace wl
{

/**
 * @struct registry
 */
struct registry : public wayland_proxy<struct wl_registry>
{
    typedef std::function<void(struct wl_registry *, uint32_t, uint32_t)> binder;
    std::unordered_map<std::string, binder> bindings;

    registry(registry const &) = delete;
    registry &operator=(registry const &) = delete;
    registry(struct wl_display *d);

    void add_global_handler(char const *iface, binder bind);

    // Events
    void global_created(uint32_t name, char const *iface, uint32_t v);
    void global_removed(uint32_t name);
};

/**
 * @struct display
 */
struct display
{
    std::unique_ptr<struct wl_display, void (*)(struct wl_display *)> d;
    struct registry r;

    display(display const &) = delete;
    display &operator=(display const &) = delete;
    display();
    bool ok() const;
    void roundtrip();
    int dispatch();
    int dispatch_pending();
    int read_events();
    void flush();
    int get_fd() const;
    int get_error();

    // Lets just proxy this for the registry
    inline void add_global_handler(char const *iface, registry::binder bind)
    {
        this->r.add_global_handler(iface, bind);
    }
};

/**
 * @struct output
 */
struct output : public wayland_proxy<struct wl_output>
{
    int width{};
    int height{};
    int physical_width{};
    int physical_height{};
    int refresh{};
    int transform{};

    output(output const &) = delete;
    output &operator=(output const &) = delete;
    output(struct wl_registry *r, uint32_t name, uint32_t v);

    // Events
    void geometry(int32_t x, int32_t y, int32_t pw, int32_t ph, int32_t subpel,
                  char const *make, char const *model, int32_t tx);
    void mode(uint32_t flags, int32_t w, int32_t h, int32_t r);
    void done();
    void scale(int32_t factor);
};
} // namespace wl

/**
 * namespace compositor
 */
namespace compositor
{

struct size
{
    uint32_t w, h;
};

struct rect
{
    int32_t w, h;
    int32_t x, y;
};

static const constexpr rect full_rect = rect{-1, -1, 0, 0};

inline bool operator==(struct rect a, struct rect b)
{
    return a.w == b.w && a.h == b.h && a.x == b.x && a.y == b.y;
}

struct controller;

struct controller_child
{
    struct controller *parent;
    uint32_t id;

    controller_child(controller_child const &) = delete;
    controller_child &operator=(controller_child const &) = delete;
    controller_child(struct controller *c, uint32_t i) : parent(c), id(i) {}
    virtual ~controller_child() {}
};

struct surface_properties
{
    uint32_t id; // let's just save an ID here too
    struct rect dst_rect;
    struct rect src_rect;
    struct size size;
    int32_t orientation;
    int32_t visibility;
    float opacity;
};

/**
 * @struct surface
 */
struct surface : public controller_child
{
    surface(surface const &) = delete;
    surface &operator=(surface const &) = delete;
    surface(uint32_t i, struct controller *c);

    // Requests
    void set_visibility(uint32_t visibility);
    void set_source_rectangle(int32_t x, int32_t y,
                              int32_t width, int32_t height);
    void set_destination_rectangle(int32_t x, int32_t y,
                                   int32_t width, int32_t height);
};

/**
 * @struct layer
 */
struct layer : public controller_child
{
    layer(layer const &) = delete;
    layer &operator=(layer const &) = delete;
    layer(uint32_t i, struct controller *c);
    layer(uint32_t i, int32_t w, int32_t h, struct controller *c);

    // Requests
    void set_visibility(uint32_t visibility);
    void set_source_rectangle(int32_t x, int32_t y, int32_t width, int32_t height);
    void set_destination_rectangle(int32_t x, int32_t y,
                                   int32_t width, int32_t height);
    void add_surface(uint32_t surface_id);
    void remove_surface(uint32_t surface_id);
};

/**
 * @struct screen
 */
struct screen : public wayland_proxy<struct ivi_wm_screen>,
                public controller_child
{
    screen(screen const &) = delete;
    screen &operator=(screen const &) = delete;
    screen(uint32_t i, struct controller *c, struct wl_output *o);

    void clear();
    void screen_created(struct screen *screen, uint32_t id);
    void set_render_order(std::vector<uint32_t> const &ro);
};

/**
 * @struct controller
 */
struct controller : public wayland_proxy<struct ivi_wm>
{
    // This controller is still missing ivi-input

    typedef std::unordered_map<uintptr_t, uint32_t> proxy_to_id_map_type;
    typedef std::unordered_map<uint32_t, std::unique_ptr<struct surface>>
        surface_map_type;
    typedef std::unordered_map<uint32_t, std::unique_ptr<struct layer>>
        layer_map_type;
    typedef std::unordered_map<uint32_t, struct screen *> screen_map_type;
    typedef std::unordered_map<uint32_t, struct surface_properties> props_map;

    // HACK:
    // The order of these member is mandatory, as when objects are destroyed
    // they will call their parent (that's us right here!) and remove their
    // proxy-to-id mapping. I.e. the *_proxy_to_id members need to be valid
    // when the surfaces/layers/screens maps are destroyed. This sucks, but
    // I cannot see a better solution w/o globals or some other horrible
    // call-our-parent construct.
    proxy_to_id_map_type surface_proxy_to_id;
    proxy_to_id_map_type layer_proxy_to_id;
    proxy_to_id_map_type screen_proxy_to_id;

    props_map sprops;
    props_map lprops;

    surface_map_type surfaces;
    layer_map_type layers;
    screen_map_type screens;

    std::unique_ptr<struct screen> screen;

    size output_size;   // Display size[pixel]
    size physical_size; // Display size[mm]

    wm::controller_hooks *chooks;

    struct wl::display *display;

    void add_proxy_to_sid_mapping(struct ivi_wm *p, uint32_t id);
    void remove_proxy_to_sid_mapping(struct ivi_wm *p);

    void add_proxy_to_lid_mapping(struct ivi_wm *p, uint32_t id);
    void remove_proxy_to_lid_mapping(struct ivi_wm *p);

    void add_proxy_to_id_mapping(struct wl_output *p, uint32_t id);
    void remove_proxy_to_id_mapping(struct wl_output *p);

    bool surface_exists(uint32_t id) const
    {
        return this->surfaces.find(id) != this->surfaces.end();
    }

    bool layer_exists(uint32_t id) const
    {
        return this->layers.find(id) != this->layers.end();
    }

    controller(struct wl_registry *r, uint32_t name, uint32_t version);

    // Requests
    void commit_changes() const
    {
        ivi_wm_commit_changes(this->proxy.get());
    }
    void layer_create(uint32_t id, int32_t w, int32_t h);
    void surface_create(uint32_t id);
    void create_screen(struct wl_output *output);

    // Events
    void surface_visibility_changed(struct surface *s, int32_t visibility);
    void surface_opacity_changed(struct surface *s, float opacity);
    void surface_source_rectangle_changed(struct surface *s, int32_t x, int32_t y,
                                          int32_t width, int32_t height);
    void surface_destination_rectangle_changed(struct surface *s, int32_t x, int32_t y,
                                               int32_t width, int32_t height);
    void surface_created(uint32_t id);
    void surface_destroyed(struct surface *s, uint32_t surface_id);
    void surface_error_detected(uint32_t object_id,
                                uint32_t error_code, char const *error_text);
    void surface_size_changed(struct surface *s, int32_t width, int32_t height);
    void surface_stats_received(struct surface *s, uint32_t surface_id,
                                uint32_t frame_count, uint32_t pid);
    void surface_added_to_layer(struct surface *s, uint32_t layer_id, uint32_t surface_id);

    void layer_visibility_changed(struct layer *l, uint32_t layer_id, int32_t visibility);
    void layer_opacity_changed(struct layer *l, uint32_t layer_id, float opacity);
    void layer_source_rectangle_changed(struct layer *l, uint32_t layer_id, int32_t x, int32_t y,
                                        int32_t width, int32_t height);
    void layer_destination_rectangle_changed(struct layer *l, uint32_t layer_id, int32_t x, int32_t y,
                                             int32_t width, int32_t height);
    void layer_created(uint32_t id);
    void layer_destroyed(struct layer *l, uint32_t layer_id);
    void layer_error_detected(uint32_t object_id,
                              uint32_t error_code, char const *error_text);
};
} // namespace compositor

#endif // !WM_WAYLAND_HPP
