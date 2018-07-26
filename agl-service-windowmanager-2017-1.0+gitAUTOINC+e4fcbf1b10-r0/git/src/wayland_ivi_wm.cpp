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

#include "wayland_ivi_wm.hpp"
#include "hmi-debug.h"

/**
 * namespace wl
 */
namespace wl
{

/**
 * display
 */
display::display()
    : d(std::unique_ptr<struct wl_display, void (*)(struct wl_display *)>(
          wl_display_connect(nullptr), &wl_display_disconnect)),
      r(d.get()) {}

bool display::ok() const { return d && wl_display_get_error(d.get()) == 0; }

void display::roundtrip() { wl_display_roundtrip(this->d.get()); }

int display::dispatch() { return wl_display_dispatch(this->d.get()); }

int display::dispatch_pending() { return wl_display_dispatch_pending(this->d.get()); }

int display::read_events()
{
    ST();
    while (wl_display_prepare_read(this->d.get()) == -1)
    {
        STN(pending_events_dispatch);
        if (wl_display_dispatch_pending(this->d.get()) == -1)
        {
            return -1;
        }
    }

    if (wl_display_flush(this->d.get()) == -1)
    {
        return -1;
    }

    if (wl_display_read_events(this->d.get()) == -1)
    {
        wl_display_cancel_read(this->d.get());
    }

    return 0;
}

void display::flush() { wl_display_flush(this->d.get()); }

int display::get_fd() const { return wl_display_get_fd(this->d.get()); }

int display::get_error() { return wl_display_get_error(this->d.get()); }

/**
 * registry
 */
namespace
{
void registry_global_created(void *data, struct wl_registry * /*r*/, uint32_t name,
                             char const *iface, uint32_t v)
{
    static_cast<struct registry *>(data)->global_created(name, iface, v);
}

void registry_global_removed(void *data, struct wl_registry * /*r*/,
                             uint32_t name)
{
    static_cast<struct registry *>(data)->global_removed(name);
}

constexpr struct wl_registry_listener registry_listener = {
    registry_global_created, registry_global_removed};
} // namespace

registry::registry(struct wl_display *d)
    : wayland_proxy(d == nullptr ? nullptr : wl_display_get_registry(d))
{
    if (this->proxy != nullptr)
    {
        wl_registry_add_listener(this->proxy.get(), &registry_listener, this);
    }
}

void registry::add_global_handler(char const *iface, binder bind)
{
    this->bindings[iface] = std::move(bind);
}

void registry::global_created(uint32_t name, char const *iface, uint32_t v)
{
    auto b = this->bindings.find(iface);
    if (b != this->bindings.end())
    {
        b->second(this->proxy.get(), name, v);
    }
    HMI_DEBUG("wm", "wl::registry @ %p global n %u i %s v %u", this->proxy.get(), name,
              iface, v);
}

void registry::global_removed(uint32_t /*name*/) {}

/**
 * output
 */
namespace
{
void output_geometry(void *data, struct wl_output * /*wl_output*/, int32_t x,
                     int32_t y, int32_t physical_width, int32_t physical_height,
                     int32_t subpixel, const char *make, const char *model,
                     int32_t transform)
{
    static_cast<struct output *>(data)->geometry(
        x, y, physical_width, physical_height, subpixel, make, model, transform);
}

void output_mode(void *data, struct wl_output * /*wl_output*/, uint32_t flags,
                 int32_t width, int32_t height, int32_t refresh)
{
    static_cast<struct output *>(data)->mode(flags, width, height, refresh);
}

void output_done(void *data, struct wl_output * /*wl_output*/)
{
    static_cast<struct output *>(data)->done();
}

void output_scale(void *data, struct wl_output * /*wl_output*/,
                  int32_t factor)
{
    static_cast<struct output *>(data)->scale(factor);
}

constexpr struct wl_output_listener output_listener = {
    output_geometry, output_mode, output_done, output_scale};
} // namespace

output::output(struct wl_registry *r, uint32_t name, uint32_t v)
    : wayland_proxy(wl_registry_bind(r, name, &wl_output_interface, v))
{
    wl_output_add_listener(this->proxy.get(), &output_listener, this);
}

void output::geometry(int32_t x, int32_t y, int32_t pw, int32_t ph,
                      int32_t subpel, char const *make, char const *model,
                      int32_t tx)
{
    HMI_DEBUG("wm",
              "wl::output %s @ %p x %i y %i w %i h %i spel %x make %s model %s tx %i",
              __func__, this->proxy.get(), x, y, pw, ph, subpel, make, model, tx);
    this->physical_width = pw;
    this->physical_height = ph;
    this->transform = tx;
}

void output::mode(uint32_t flags, int32_t w, int32_t h, int32_t r)
{
    HMI_DEBUG("wm", "wl::output %s @ %p f %x w %i h %i r %i", __func__,
              this->proxy.get(), flags, w, h, r);
    if ((flags & WL_OUTPUT_MODE_CURRENT) != 0u)
    {
        this->width = w;
        this->height = h;
        this->refresh = r;
    }
}

void output::done()
{
    HMI_DEBUG("wm", "wl::output %s @ %p done", __func__, this->proxy.get());
    // Pivot and flipped
    if (this->transform == WL_OUTPUT_TRANSFORM_90 ||
        this->transform == WL_OUTPUT_TRANSFORM_270 ||
        this->transform == WL_OUTPUT_TRANSFORM_FLIPPED_90 ||
        this->transform == WL_OUTPUT_TRANSFORM_FLIPPED_270)
    {
        std::swap(this->width, this->height);
        std::swap(this->physical_width, this->physical_height);
    }
}

void output::scale(int32_t factor)
{
    HMI_DEBUG("wm", "wl::output %s @ %p f %i", __func__, this->proxy.get(), factor);
}
} // namespace wl

/**
 * namespace compositor
 */
namespace compositor
{

namespace
{

void surface_visibility_changed(
    void *data, struct ivi_wm * /*ivi_wm*/,
    uint32_t surface_id, int32_t visibility)
{
    auto s = static_cast<struct surface *>(data);
    s->parent->surface_visibility_changed(s, visibility);
}

void surface_opacity_changed(void *data, struct ivi_wm * /*ivi_wm*/,
                             uint32_t surface_id, wl_fixed_t opacity)
{
    auto s = static_cast<struct surface *>(data);
    s->parent->surface_opacity_changed(s, float(wl_fixed_to_double(opacity)));
}

void surface_source_rectangle_changed(
    void *data, struct ivi_wm * /*ivi_wm*/, uint32_t surface_id,
    int32_t x, int32_t y, int32_t width, int32_t height)
{
    auto s = static_cast<struct surface *>(data);
    s->parent->surface_source_rectangle_changed(s, x, y, width, height);
}

void surface_destination_rectangle_changed(
    void *data, struct ivi_wm * /*ivi_wm*/, uint32_t surface_id,
    int32_t x, int32_t y, int32_t width, int32_t height)
{
    auto s = static_cast<struct surface *>(data);
    s->parent->surface_destination_rectangle_changed(s, x, y, width, height);
}

void surface_created(void *data, struct ivi_wm * /*ivi_wm*/,
                     uint32_t id_surface)
{
    static_cast<struct controller *>(data)->surface_created(id_surface);
}

void surface_destroyed(
    void *data, struct ivi_wm * /*ivi_wm*/, uint32_t surface_id)
{
    auto s = static_cast<struct surface *>(data);
    s->parent->surface_destroyed(s, surface_id);
}

void surface_error_detected(void *data, struct ivi_wm * /*ivi_wm*/, uint32_t object_id,
                            uint32_t error_code, const char *error_text)
{
    static_cast<struct controller *>(data)->surface_error_detected(
        object_id, error_code, error_text);
}

void surface_size_changed(
    void *data, struct ivi_wm * /*ivi_wm*/, uint32_t surface_id,
    int32_t width, int32_t height)
{
    auto s = static_cast<struct surface *>(data);
    s->parent->surface_size_changed(s, width, height);
}

void surface_stats_received(void *data, struct ivi_wm * /*ivi_wm*/,
                            uint32_t surface_id, uint32_t frame_count, uint32_t pid)
{
    auto s = static_cast<struct surface *>(data);
    s->parent->surface_stats_received(s, surface_id, frame_count, pid);
}

void surface_added_to_layer(void *data, struct ivi_wm * /*ivi_wm*/,
                            uint32_t layer_id, uint32_t surface_id)
{
    auto s = static_cast<struct surface *>(data);
    s->parent->surface_added_to_layer(s, layer_id, surface_id);
}

void layer_visibility_changed(void *data, struct ivi_wm * /*ivi_wm*/,
                              uint32_t layer_id, int32_t visibility)
{
    auto l = static_cast<struct layer *>(data);
    l->parent->layer_visibility_changed(l, layer_id, visibility);
}

void layer_opacity_changed(void *data, struct ivi_wm * /*ivi_wm*/,
                           uint32_t layer_id, wl_fixed_t opacity)
{
    auto l = static_cast<struct layer *>(data);
    l->parent->layer_opacity_changed(l, layer_id, float(wl_fixed_to_double(opacity)));
}

void layer_source_rectangle_changed(
    void *data, struct ivi_wm * /*ivi_wm*/, uint32_t layer_id,
    int32_t x, int32_t y, int32_t width, int32_t height)
{
    auto l = static_cast<struct layer *>(data);
    l->parent->layer_source_rectangle_changed(l, layer_id, x, y, width, height);
}

void layer_destination_rectangle_changed(
    void *data, struct ivi_wm * /*ivi_wm*/, uint32_t layer_id,
    int32_t x, int32_t y, int32_t width, int32_t height)
{
    auto l = static_cast<struct layer *>(data);
    l->parent->layer_destination_rectangle_changed(l, layer_id, x, y, width, height);
}

void layer_created(void *data, struct ivi_wm * /*ivi_wm*/,
                   uint32_t id_layer)
{
    static_cast<struct controller *>(data)->layer_created(id_layer);
}

void layer_destroyed(void *data, struct ivi_wm * /*ivi_wm*/, uint32_t layer_id)
{
    auto l = static_cast<struct layer *>(data);
    l->parent->layer_destroyed(l, layer_id);
}

void layer_error_detected(void *data, struct ivi_wm * /*ivi_wm*/, uint32_t object_id,
                          uint32_t error_code, const char *error_text)
{
    static_cast<struct controller *>(data)->layer_error_detected(
        object_id, error_code, error_text);
}

constexpr struct ivi_wm_listener listener = {
    surface_visibility_changed,
    layer_visibility_changed,
    surface_opacity_changed,
    layer_opacity_changed,
    surface_source_rectangle_changed,
    layer_source_rectangle_changed,
    surface_destination_rectangle_changed,
    layer_destination_rectangle_changed,
    surface_created,
    layer_created,
    surface_destroyed,
    layer_destroyed,
    surface_error_detected,
    layer_error_detected,
    surface_size_changed,
    surface_stats_received,
    surface_added_to_layer,
};

void screen_created(void *data, struct ivi_wm_screen *ivi_wm_screen, uint32_t id)
{
    static_cast<struct screen *>(data)->screen_created((struct screen *)data, id);
}

void layer_added(void *data,
                 struct ivi_wm_screen *ivi_wm_screen,
                 uint32_t layer_id)
{
    HMI_DEBUG("wm", "added layer_id:%d", layer_id);
}

void connector_name(void *data,
                    struct ivi_wm_screen *ivi_wm_screen,
                    const char *process_name)
{
    HMI_DEBUG("wm", "process_name:%s", process_name);
}

void screen_error(void *data,
                  struct ivi_wm_screen *ivi_wm_screen,
                  uint32_t error,
                  const char *message)
{
    HMI_DEBUG("wm", "screen error:%d message:%s", error, message);
}

constexpr struct ivi_wm_screen_listener screen_listener = {
    screen_created,
    layer_added,
    connector_name,
    screen_error,
};
} // namespace

/**
 * surface
 */
surface::surface(uint32_t i, struct controller *c)
    : controller_child(c, i)
{
    this->parent->add_proxy_to_sid_mapping(this->parent->proxy.get(), i);
}

void surface::set_visibility(uint32_t visibility)
{
    HMI_DEBUG("wm", "compositor::surface id:%d v:%d", this->id, visibility);
    ivi_wm_set_surface_visibility(this->parent->proxy.get(), this->id, visibility);
}

void surface::set_source_rectangle(int32_t x, int32_t y,
                                   int32_t width, int32_t height)
{
    ivi_wm_set_surface_source_rectangle(this->parent->proxy.get(), this->id,
                                        x, y, width, height);
}

void surface::set_destination_rectangle(int32_t x, int32_t y,
                                        int32_t width, int32_t height)
{
    ivi_wm_set_surface_destination_rectangle(this->parent->proxy.get(), this->id,
                                             x, y, width, height);
}

/**
 * layer
 */
layer::layer(uint32_t i, struct controller *c) : layer(i, 0, 0, c) {}

layer::layer(uint32_t i, int32_t w, int32_t h, struct controller *c)
    : controller_child(c, i)
{
    this->parent->add_proxy_to_lid_mapping(this->parent->proxy.get(), i);
    ivi_wm_create_layout_layer(c->proxy.get(), i, w, h);
}

void layer::set_visibility(uint32_t visibility)
{
    ivi_wm_set_layer_visibility(this->parent->proxy.get(), this->id, visibility);
}

void layer::set_source_rectangle(int32_t x, int32_t y, int32_t width, int32_t height)
{
    ivi_wm_set_layer_source_rectangle(this->parent->proxy.get(), this->id, x, y, width, height);
}

void layer::set_destination_rectangle(int32_t x, int32_t y,
                                      int32_t width, int32_t height)
{
    ivi_wm_set_layer_destination_rectangle(this->parent->proxy.get(), this->id,
                                           x, y, width, height);
}

void layer::add_surface(uint32_t surface_id)
{
    ivi_wm_layer_add_surface(this->parent->proxy.get(), this->id, surface_id);
}

void layer::remove_surface(uint32_t surface_id)
{
    ivi_wm_layer_remove_surface(this->parent->proxy.get(), this->id, surface_id);
}

/**
 * screen
 */
screen::screen(uint32_t i, struct controller *c, struct wl_output *o)
    : wayland_proxy(ivi_wm_create_screen(c->proxy.get(), o)),
      controller_child(c, i)
{
    HMI_DEBUG("wm", "compositor::screen @ %p id %u o %p", this->proxy.get(), i, o);

    // Add listener for screen
    ivi_wm_screen_add_listener(this->proxy.get(), &screen_listener, this);
}

void screen::clear() { ivi_wm_screen_clear(this->proxy.get()); }

void screen::screen_created(struct screen *screen, uint32_t id)
{
    HMI_DEBUG("wm", "compositor::screen @ %p screen %u (%x) @ %p", this->proxy.get(),
              id, id, screen);
    this->id = id;
    this->parent->screens[id] = screen;
}

void screen::set_render_order(std::vector<uint32_t> const &ro)
{
    std::size_t i;

    // Remove all layers from the screen render order
    ivi_wm_screen_clear(this->proxy.get());

    for (i = 0; i < ro.size(); i++)
    {
        HMI_DEBUG("wm", "compositor::screen @ %p add layer %u", this->proxy.get(), ro[i]);
        // Add the layer to screen render order at nearest z-position
        ivi_wm_screen_add_layer(this->proxy.get(), ro[i]);
    }
}

/**
 * controller
 */
controller::controller(struct wl_registry *r, uint32_t name, uint32_t version)
    : wayland_proxy(
          wl_registry_bind(r, name, &ivi_wm_interface, version)),
      output_size{}
{
    ivi_wm_add_listener(this->proxy.get(), &listener, this);
}

void controller::layer_create(uint32_t id, int32_t w, int32_t h)
{
    this->layers[id] = std::make_unique<struct layer>(id, w, h, this);
}

void controller::surface_create(uint32_t id)
{
    this->surfaces[id] = std::make_unique<struct surface>(id, this);

    // TODO: If Clipping is necessary, this process should be modified.
    {
        // Set surface type:IVI_WM_SURFACE_TYPE_DESKTOP)
        // for resizing wayland surface when switching from split to full surface.
        ivi_wm_set_surface_type(this->proxy.get(), id, IVI_WM_SURFACE_TYPE_DESKTOP);

        // Set source reactangle even if we should not need to set it
        // for enable setting for destination region.
        this->surfaces[id]->set_source_rectangle(0, 0, this->output_size.w, this->output_size.h);

        // Flush display
        this->display->flush();
    }
}

void controller::create_screen(struct wl_output *output)
{
    // TODO: screen id is 0 (WM manages one screen for now)
    this->screen = std::make_unique<struct screen>(0, this, output);
}

void controller::layer_created(uint32_t id)
{
    HMI_DEBUG("wm", "compositor::controller @ %p layer %u (%x)", this->proxy.get(), id, id);
    if (this->layers.find(id) != this->layers.end())
    {
        HMI_DEBUG("wm", "WindowManager has created layer %u (%x) already", id, id);
    }
    else
    {
        this->layers[id] = std::make_unique<struct layer>(id, this);
    }
}

void controller::layer_error_detected(uint32_t object_id,
                                      uint32_t error_code, const char *error_text)
{
    HMI_DEBUG("wm", "compositor::controller @ %p error o %d c %d text %s",
              this->proxy.get(), object_id, error_code, error_text);
}

void controller::surface_visibility_changed(struct surface *s, int32_t visibility)
{
    HMI_DEBUG("wm", "compositor::surface %s @ %d v %i", __func__, s->id,
              visibility);
    this->sprops[s->id].visibility = visibility;
    this->chooks->surface_visibility(s->id, visibility);
}

void controller::surface_opacity_changed(struct surface *s, float opacity)
{
    HMI_DEBUG("wm", "compositor::surface %s @ %d o %f", __func__, s->id,
              opacity);
    this->sprops[s->id].opacity = opacity;
}

void controller::surface_source_rectangle_changed(struct surface *s, int32_t x,
                                                  int32_t y, int32_t width,
                                                  int32_t height)
{
    HMI_DEBUG("wm", "compositor::surface %s @ %d x %i y %i w %i h %i", __func__,
              s->id, x, y, width, height);
    this->sprops[s->id].src_rect = rect{width, height, x, y};
}

void controller::surface_destination_rectangle_changed(struct surface *s, int32_t x,
                                                       int32_t y, int32_t width,
                                                       int32_t height)
{
    HMI_DEBUG("wm", "compositor::surface %s @ %d x %i y %i w %i h %i", __func__,
              s->id, x, y, width, height);
    this->sprops[s->id].dst_rect = rect{width, height, x, y};
    this->chooks->surface_destination_rectangle(s->id, x, y, width, height);
}

void controller::surface_size_changed(struct surface *s, int32_t width,
                                      int32_t height)
{
    HMI_DEBUG("wm", "compositor::surface %s @ %d w %i h %i", __func__, s->id,
              width, height);
    this->sprops[s->id].size = size{uint32_t(width), uint32_t(height)};
}

void controller::surface_added_to_layer(struct surface *s,
                                        uint32_t layer_id, uint32_t surface_id)
{
    HMI_DEBUG("wm", "compositor::surface %s @ %d l %u",
              __func__, layer_id, surface_id);
}

void controller::surface_stats_received(struct surface *s, uint32_t surface_id,
                                        uint32_t frame_count, uint32_t pid)
{
    HMI_DEBUG("wm", "compositor::surface %s @ %d f %u pid %u",
              __func__, surface_id, frame_count, pid);
}

void controller::surface_created(uint32_t id)
{
    HMI_DEBUG("wm", "compositor::controller @ %p surface %u (%x)", this->proxy.get(), id,
              id);
    if (this->surfaces.find(id) == this->surfaces.end())
    {
        this->surfaces[id] = std::make_unique<struct surface>(id, this);
        this->chooks->surface_created(id);

        // Set surface type:IVI_WM_SURFACE_TYPE_DESKTOP)
        // for resizing wayland surface when switching from split to full surface.
        ivi_wm_set_surface_type(this->proxy.get(), id, IVI_WM_SURFACE_TYPE_DESKTOP);

        // Flush display
        this->display->flush();
    }
}

void controller::surface_destroyed(struct surface *s, uint32_t surface_id)
{
    HMI_DEBUG("wm", "compositor::surface %s @ %d", __func__, surface_id);
    this->chooks->surface_removed(surface_id);
    this->sprops.erase(surface_id);
    this->surfaces.erase(surface_id);
}

void controller::surface_error_detected(uint32_t object_id,
                                        uint32_t error_code, const char *error_text)
{
    HMI_DEBUG("wm", "compositor::controller @ %p error o %d c %d text %s",
              this->proxy.get(), object_id, error_code, error_text);
}

void controller::layer_visibility_changed(struct layer *l, uint32_t layer_id, int32_t visibility)
{
    HMI_DEBUG("wm", "compositor::layer %s @ %d v %i", __func__, layer_id, visibility);
    this->lprops[layer_id].visibility = visibility;
}

void controller::layer_opacity_changed(struct layer *l, uint32_t layer_id, float opacity)
{
    HMI_DEBUG("wm", "compositor::layer %s @ %d o %f", __func__, layer_id, opacity);
    this->lprops[layer_id].opacity = opacity;
}

void controller::layer_source_rectangle_changed(struct layer *l, uint32_t layer_id,
                                                int32_t x, int32_t y,
                                                int32_t width, int32_t height)
{
    HMI_DEBUG("wm", "compositor::layer %s @ %d x %i y %i w %i h %i",
              __func__, layer_id, x, y, width, height);
    this->lprops[layer_id].src_rect = rect{width, height, x, y};
}

void controller::layer_destination_rectangle_changed(struct layer *l, uint32_t layer_id,
                                                     int32_t x, int32_t y,
                                                     int32_t width, int32_t height)
{
    HMI_DEBUG("wm", "compositor::layer %s @ %d x %i y %i w %i h %i",
              __func__, layer_id, x, y, width, height);
    this->lprops[layer_id].dst_rect = rect{width, height, x, y};
}

void controller::layer_destroyed(struct layer *l, uint32_t layer_id)
{
    HMI_DEBUG("wm", "compositor::layer %s @ %d", __func__, layer_id);
    this->lprops.erase(layer_id);
    this->layers.erase(layer_id);
}

void controller::add_proxy_to_sid_mapping(struct ivi_wm *p,
                                          uint32_t id)
{
    HMI_DEBUG("wm", "Add surface proxy mapping for %p (%u)", p, id);
    this->surface_proxy_to_id[uintptr_t(p)] = id;
    this->sprops[id].id = id;
}

void controller::remove_proxy_to_sid_mapping(struct ivi_wm *p)
{
    HMI_DEBUG("wm", "Remove surface proxy mapping for %p", p);
    this->surface_proxy_to_id.erase(uintptr_t(p));
}

void controller::add_proxy_to_lid_mapping(struct ivi_wm *p,
                                          uint32_t id)
{
    HMI_DEBUG("wm", "Add layer proxy mapping for %p (%u)", p, id);
    this->layer_proxy_to_id[uintptr_t(p)] = id;
    this->lprops[id].id = id;
}

void controller::remove_proxy_to_lid_mapping(struct ivi_wm *p)
{
    HMI_DEBUG("wm", "Remove layer proxy mapping for %p", p);
    this->layer_proxy_to_id.erase(uintptr_t(p));
}

void controller::add_proxy_to_id_mapping(struct wl_output *p, uint32_t id)
{
    HMI_DEBUG("wm", "Add screen proxy mapping for %p (%u)", p, id);
    this->screen_proxy_to_id[uintptr_t(p)] = id;
}

void controller::remove_proxy_to_id_mapping(struct wl_output *p)
{
    HMI_DEBUG("wm", "Remove screen proxy mapping for %p", p);
    this->screen_proxy_to_id.erase(uintptr_t(p));
}

} // namespace compositor
