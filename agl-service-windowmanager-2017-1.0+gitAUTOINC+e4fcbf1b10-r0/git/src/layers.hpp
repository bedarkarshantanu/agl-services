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

#ifndef TMCAGLWM_LAYERS_H
#define TMCAGLWM_LAYERS_H

#include <string>

#include "../include/json.hpp"
#include "layout.hpp"
#include "result.hpp"
#include "wayland_ivi_wm.hpp"

namespace wm
{

struct split_layout
{
    std::string name;
    std::string main_match;
    std::string sub_match;
};

struct layer
{
    using json = nlohmann::json;

    // A more or less descriptive name?
    std::string name = "";
    // The actual layer ID
    int layer_id = -1;
    // The rectangular region surfaces are allowed to draw on
    // this layer, note however, width and hieght of the rect
    // can be negative, in which case they specify that
    // the actual value is computed using MAX + 1 - w
    // That is; allow us to specify dimensions dependent on
    // e.g. screen dimension, w/o knowing the actual screen size.
    compositor::rect rect;
    // Specify a role prefix for surfaces that should be
    // put on this layer.
    std::string role;
    // TODO: perhaps a zorder is needed here?
    std::vector<struct split_layout> layouts;

    mutable struct LayoutState state;

    // Flag of normal layout only
    bool is_normal_layout_only;

    explicit layer(nlohmann::json const &j);

    json to_json() const;
};

struct layer_map
{
    using json = nlohmann::json;

    using storage_type = std::map<int, struct layer>;
    using layers_type = std::vector<uint32_t>;
    using role_to_layer_map = std::vector<std::pair<std::string, int>>;
    using addsurf_layer_map = std::map<int, int>;

    storage_type mapping; // map surface_id to layer
    layers_type layers;   // the actual layer IDs we have
    int main_surface;
    std::string main_surface_name;
    role_to_layer_map roles;
    addsurf_layer_map surfaces; // additional surfaces on layers

    optional<int> get_layer_id(int surface_id);
    optional<int> get_layer_id(std::string const &role);
    optional<struct LayoutState *> get_layout_state(int surface_id)
    {
        int layer_id = *this->get_layer_id(surface_id);
        auto i = this->mapping.find(layer_id);
        return i == this->mapping.end()
                   ? nullopt
                   : optional<struct LayoutState *>(&i->second.state);
    }
    optional<struct layer> get_layer(int layer_id)
    {
        auto i = this->mapping.find(layer_id);
        return i == this->mapping.end() ? nullopt
                                        : optional<struct layer>(i->second);
    }

    layers_type::size_type get_layers_count() const
    {
        return this->layers.size();
    }

    void add_surface(int surface_id, int layer_id)
    {
        this->surfaces[surface_id] = layer_id;
    }

    void remove_surface(int surface_id)
    {
        this->surfaces.erase(surface_id);
    }

    json to_json() const;
    compositor::rect getAreaSize(const std::string &area);
    const compositor::rect getScaleDestRect(int output_w, int output_h, const std::string &aspect_setting);
    int loadAreaDb();

  private:
    std::unordered_map<std::string, compositor::rect> area2size;

    static const char *kDefaultAreaDb;
};

struct result<struct layer_map> to_layer_map(nlohmann::json const &j);

static const nlohmann::json default_layers_json = {
   {"main_surface", {
      {"surface_role", "HomeScreen"}
   }},
   {"mappings", {
      {
         {"role", "^HomeScreen$"},
         {"name", "HomeScreen"},
         {"layer_id", 1000},
         {"area", {
            {"type", "full"}
         }}
      },
      {
         {"role", "MediaPlayer|Radio|Phone|Navigation|HVAC|Settings|Dashboard|POI|Mixer"},
         {"name", "apps"},
         {"layer_id", 1001},
         {"area", {
            {"type", "rect"},
            {"rect", {
               {"x", 0},
               {"y", 218},
               {"width", -1},
               {"height", -433}
            }}
         }}
      },
      {
         {"role", "^OnScreen.*"},
         {"name", "popups"},
         {"layer_id", 9999},
         {"area", {
            {"type", "rect"},
            {"rect", {
               {"x", 0},
               {"y", 760},
               {"width", -1},
               {"height", 400}
            }}
         }}
      }
   }}
};
} // namespace wm

#endif // TMCAGLWM_LAYERS_H
