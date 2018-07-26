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

#ifndef TMCAGLWM_CONTROLLER_HOOKS_HPP
#define TMCAGLWM_CONTROLLER_HOOKS_HPP

#include <cstdint>

#include <functional>

namespace wm
{

class WindowManager;

struct controller_hooks
{
    WindowManager *app;

    void surface_created(uint32_t surface_id);

    void surface_removed(uint32_t surface_id);
    void surface_visibility(uint32_t surface_id, uint32_t v);
    void surface_destination_rectangle(uint32_t surface_id, uint32_t x, uint32_t y, uint32_t w, uint32_t h);
};

} // namespace wm

#endif // TMCAGLWM_CONTROLLER_HOOKS_HPP
