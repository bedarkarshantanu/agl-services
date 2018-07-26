#
# Copyright (c) 2017 TOYOTA MOTOR CORPORATION
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

project(HomeScreenService C CXX)

cmake_minimum_required(VERSION 3.0)

set(PROJECT_NAME "HomeScreen Service 2017")
set(PROJECT_VERSION "1.0")
set(PROJECT_PRETTY_NAME "HM")
set(PROJECT_DESCRIPTION "HomeScreen Service 2017")
set(PROJECT_INCLUDEDIR ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR})
set(PROJECT_LIBDIR ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR})
set(binding_install_dir ${PREFIX}/lib/homescreen)
set(PROJECT_BINDINGDIR ${binding_install_dir})

# get pkgconfig
INCLUDE(FindPkgConfig)
link_libraries(-Wl,--as-needed -Wl,--gc-sections -Wl,--no-undefined)

set(CMAKE_BUILD_TYPE Debug)
set(DEBUGMODE "1")
add_definitions(-DDEBUGMODE)

add_subdirectory(src)
