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

project(af-steering-wheel-binding C)

cmake_minimum_required(VERSION 2.8)

include(GNUInstallDirs)

set(PROJECT_NAME "af-steering-wheel-binding")
set(PROJECT_PRETTY_NAME "af binding for steering wheel")
set(PROJECT_DESCRIPTION "Binding for handling Vehicle signalisation of steering wheel")
set(PROJECT_VERSION "1.0")

#set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_BUILD_TYPE Release)

add_subdirectory(src)


