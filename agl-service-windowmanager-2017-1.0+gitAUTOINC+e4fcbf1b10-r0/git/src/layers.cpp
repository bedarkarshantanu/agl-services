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

#include <regex>

#include "layers.hpp"
#include "json_helper.hpp"
#include "hmi-debug.h"

namespace wm
{

using json = nlohmann::json;

layer::layer(nlohmann::json const &j)
{
    this->role = j["role"];
    this->name = j["name"];
    this->layer_id = j["layer_id"];

    // Init flag of normal layout only
    this->is_normal_layout_only = true;

    auto split_layouts = j.find("split_layouts");
    if (split_layouts != j.end())
    {

        // Clear flag of normal layout only
        this->is_normal_layout_only = false;

        auto &sls = j["split_layouts"];
        // this->layouts.reserve(sls.size());
        std::transform(std::cbegin(sls), std::cend(sls),
                       std::back_inserter(this->layouts), [this](json const &sl) {
                           struct split_layout l
                           {
                               sl["name"], sl["main_match"], sl["sub_match"]
                           };
                           HMI_DEBUG("wm",
                                     "layer %d add split_layout \"%s\" (main: \"%s\") (sub: "
                                     "\"%s\")",
                                     this->layer_id,
                                     l.name.c_str(), l.main_match.c_str(),
                                     l.sub_match.c_str());
                           return l;
                       });
    }
    HMI_DEBUG("wm", "layer_id:%d is_normal_layout_only:%d\n",
              this->layer_id, this->is_normal_layout_only);
}

struct result<struct layer_map> to_layer_map(nlohmann::json const &j)
{
    try
    {
        layer_map stl{};
        auto m = j["mappings"];

        std::transform(std::cbegin(m), std::cend(m),
                       std::inserter(stl.mapping, stl.mapping.end()),
                       [](nlohmann::json const &j) {
                           return std::pair<int, struct layer>(
                               j.value("layer_id", -1), layer(j));
                       });

        // TODO: add sanity checks here?
        // * check for double IDs
        // * check for double names/roles

        stl.layers.reserve(m.size());
        std::transform(std::cbegin(stl.mapping), std::cend(stl.mapping),
                       std::back_inserter(stl.layers),
                       [&stl](std::pair<int, struct layer> const &k) {
                           stl.roles.emplace_back(
                               std::make_pair(k.second.role, k.second.layer_id));
                           return unsigned(k.second.layer_id);
                       });

        std::sort(stl.layers.begin(), stl.layers.end());

        for (auto i : stl.mapping)
        {
            if (i.second.name.empty())
            {
                return Err<struct layer_map>("Found mapping w/o name");
            }
            if (i.second.layer_id == -1)
            {
                return Err<struct layer_map>("Found invalid/unset IDs in mapping");
            }
        }

        auto msi = j.find("main_surface");
        if (msi != j.end())
        {
            stl.main_surface_name = msi->value("surface_role", "");
            stl.main_surface = -1;
        }

        return Ok(stl);
    }
    catch (std::exception &e)
    {
        return Err<struct layer_map>(e.what());
    }
}

optional<int>
layer_map::get_layer_id(int surface_id)
{
    auto i = this->surfaces.find(surface_id);
    if (i != this->surfaces.end())
    {
        return optional<int>(i->second);
    }
    return nullopt;
}

optional<int> layer_map::get_layer_id(std::string const &role)
{
    for (auto const &r : this->roles)
    {
        auto re = std::regex(r.first);
        if (std::regex_match(role, re))
        {
            HMI_DEBUG("wm", "role %s matches layer %d", role.c_str(), r.second);
            return optional<int>(r.second);
        }
    }
    HMI_DEBUG("wm", "role %s does NOT match any layer", role.c_str());
    return nullopt;
}

json layer::to_json() const
{
    auto is_full = this->rect == compositor::full_rect;

    json r{};
    if (is_full)
    {
        r = {{"type", "full"}};
    }
    else
    {
        r = {{"type", "rect"},
             {"rect",
              {{"x", this->rect.x},
               {"y", this->rect.y},
               {"width", this->rect.w},
               {"height", this->rect.h}}}};
    }

    return {
        {"name", this->name},
        {"role", this->role},
        {"layer_id", this->layer_id},
        {"area", r},
    };
}

json layer_map::to_json() const
{
    json j{};
    for (auto const &i : this->mapping)
    {
        j.push_back(i.second.to_json());
    }
    return j;
}

compositor::rect layer_map::getAreaSize(const std::string &area)
{
    return area2size[area];
}

const compositor::rect layer_map::getScaleDestRect(
    int to_w, int to_h, const std::string &aspect_setting)
{
    compositor::rect rct;
    rct.x = 0;
    rct.y = 0;
    rct.w = to_w;
    rct.h = to_h;
    HMI_NOTICE("wm:lm",
               "Scaling:'%s'. Check 'fullscreen' is set.", aspect_setting.c_str());
    // Base is "fullscreen". Crash me if "fullscreen" is not set
    compositor::rect base = this->area2size.at("fullscreen");
    HMI_DEBUG("wm:lm", "Output size, width: %d, height: %d / fullscreen width: %d, height: %d",
              to_w, to_h, base.w, base.h);
    // If full_rct.w or full_rct.h == 0, crash me on purpose
    double scale_rate_w = double(to_w) / double(base.w);
    double scale_rate_h = double(to_h) / double(base.h);
    double scale;
    if (scale_rate_h < scale_rate_w)
    {
        scale = scale_rate_h;
    }
    else
    {
        scale = scale_rate_w;
    }
    HMI_DEBUG("wm", "set scale: %5.2f", scale);

    // Scaling
    if (aspect_setting == "aspect_fit")
    {
        // offset
        rct.x = (to_w - scale * base.w) / 2;
        rct.y = (to_h - scale * base.h) / 2;

        // scaling
        rct.w = base.w * scale;
        rct.h = base.h * scale;
    }
    else if (aspect_setting == "display_fit")
    {
        // offset is none
        // scaling
        rct.w = base.w * scale_rate_w;
        rct.h = base.h * scale_rate_h;
    }
    HMI_DEBUG("wm:lm", "offset x: %d, y: %d", rct.x, rct.y);
    HMI_DEBUG("wm:lm", "after scaling w: %d, h: %d", rct.w, rct.h);
    return rct;
}

int layer_map::loadAreaDb()
{
    HMI_DEBUG("wm:lm", "Call");

    // Get afm application installed dir
    char const *afm_app_install_dir = getenv("AFM_APP_INSTALL_DIR");
    HMI_DEBUG("wm:lm", "afm_app_install_dir:%s", afm_app_install_dir);

    std::string file_name;
    if (!afm_app_install_dir)
    {
        HMI_ERROR("wm:lm", "AFM_APP_INSTALL_DIR is not defined");
    }
    else
    {
        file_name = std::string(afm_app_install_dir) + std::string("/etc/areas.db");
    }

    // Load area.db
    json_object *json_obj;
    int ret = jh::inputJsonFilie(file_name.c_str(), &json_obj);
    if (0 > ret)
    {
        HMI_DEBUG("wm:lm", "Could not open area.db, so use default area information");
        json_obj = json_tokener_parse(kDefaultAreaDb);
    }
    HMI_DEBUG("wm:lm", "json_obj dump:%s", json_object_get_string(json_obj));

    // Perse areas
    HMI_DEBUG("wm:lm", "Perse areas");
    json_object *json_cfg;
    if (!json_object_object_get_ex(json_obj, "areas", &json_cfg))
    {
        HMI_ERROR("wm:lm", "Parse Error!!");
        return -1;
    }

    int len = json_object_array_length(json_cfg);
    HMI_DEBUG("wm:lm", "json_cfg len:%d", len);
    HMI_DEBUG("wm:lm", "json_cfg dump:%s", json_object_get_string(json_cfg));

    const char *area;
    for (int i = 0; i < len; i++)
    {
        json_object *json_tmp = json_object_array_get_idx(json_cfg, i);
        HMI_DEBUG("wm:lm", "> json_tmp dump:%s", json_object_get_string(json_tmp));

        area = jh::getStringFromJson(json_tmp, "name");
        if (nullptr == area)
        {
            HMI_ERROR("wm:lm", "Parse Error!!");
            return -1;
        }
        HMI_DEBUG("wm:lm", "> area:%s", area);

        json_object *json_rect;
        if (!json_object_object_get_ex(json_tmp, "rect", &json_rect))
        {
            HMI_ERROR("wm:lm", "Parse Error!!");
            return -1;
        }
        HMI_DEBUG("wm:lm", "> json_rect dump:%s", json_object_get_string(json_rect));

        compositor::rect area_size;
        area_size.x = jh::getIntFromJson(json_rect, "x");
        area_size.y = jh::getIntFromJson(json_rect, "y");
        area_size.w = jh::getIntFromJson(json_rect, "w");
        area_size.h = jh::getIntFromJson(json_rect, "h");

        this->area2size[area] = area_size;
    }

    // Check
    for (auto itr = this->area2size.begin();
         itr != this->area2size.end(); ++itr)
    {
        HMI_DEBUG("wm:lm", "area:%s x:%d y:%d w:%d h:%d",
                  itr->first.c_str(), itr->second.x, itr->second.y,
                  itr->second.w, itr->second.h);
    }

    // Release json_object
    json_object_put(json_obj);

    return 0;
}

const char* layer_map::kDefaultAreaDb = "{ \
    \"areas\": [ \
        { \
            \"name\": \"fullscreen\", \
            \"rect\": { \
                \"x\": 0, \
                \"y\": 0, \
                \"w\": 1080, \
                \"h\": 1920 \
            } \
        }, \
        { \
            \"name\": \"normal.full\", \
            \"rect\": { \
                \"x\": 0, \
                \"y\": 218, \
                \"w\": 1080, \
                \"h\": 1488 \
            } \
        }, \
        { \
            \"name\": \"split.main\", \
            \"rect\": { \
                \"x\": 0, \
                \"y\": 218, \
                \"w\": 1080, \
                \"h\": 744 \
            } \
        }, \
        { \
            \"name\": \"split.sub\", \
            \"rect\": { \
                \"x\": 0, \
                \"y\": 962, \
                \"w\": 1080, \
                \"h\": 744 \
            } \
        }, \
        { \
            \"name\": \"software_keyboard\", \
            \"rect\": { \
                \"x\": 0, \
                \"y\": 962, \
                \"w\": 1080, \
                \"h\": 744 \
            } \
        }, \
        { \
            \"name\": \"restriction.normal\", \
            \"rect\": { \
                \"x\": 0, \
                \"y\": 218, \
                \"w\": 1080, \
                \"h\": 1488 \
            } \
        }, \
        { \
            \"name\": \"restriction.split.main\", \
            \"rect\": { \
                \"x\": 0, \
                \"y\": 218, \
                \"w\": 1080, \
                \"h\": 744 \
            } \
        }, \
        { \
            \"name\": \"restriction.split.sub\", \
            \"rect\": { \
                \"x\": 0, \
                \"y\": 962, \
                \"w\": 1080, \
                \"h\": 744 \
            } \
        }, \
        { \
            \"name\": \"on_screen\", \
            \"rect\": { \
                \"x\": 0, \
                \"y\": 218, \
                \"w\": 1080, \
                \"h\": 1488 \
            } \
        } \
    ] \
}";

} // namespace wm
