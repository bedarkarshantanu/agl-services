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

wlproto(IVI_CON ivi-wm)

include(FindPkgConfig)
pkg_check_modules(AFB REQUIRED afb-daemon)
pkg_check_modules(SD REQUIRED libsystemd>=222)

# We do not want a prefix for our module
set(CMAKE_SHARED_MODULE_PREFIX "")

set(TARGETS_WM windowmanager-service)

add_library(${TARGETS_WM} MODULE
   main.cpp
   wayland_ivi_wm.cpp
   util.cpp
   layout.cpp
   ${IVI_CON_PROTO}
   json_helper.cpp
   window_manager.cpp
   layers.cpp
   wm_client.cpp
   wm_error.cpp
   wm_config.cpp
   applist.cpp
   request.cpp)

target_include_directories(${TARGETS_WM}
    PRIVATE
        ${AFB_INCLUDE_DIRS}
        ${SD_INCLUDE_DIRS}
        ../include
        ../src)

target_link_libraries(${TARGETS_WM}
    PRIVATE
        ${AFB_LIBRARIES}
        ${WLC_LIBRARIES}
        ${SD_LIBRARIES})

target_compile_definitions(${TARGETS_WM}
    PRIVATE
        AFB_BINDING_VERSION=2
        # We do not want source location of messages
        AFB_BINDING_PRAGMA_NO_VERBOSE_DETAILS
        WINMAN_VERSION_STRING="${PACKAGE_VERSION}"
        _GNU_SOURCE)

if(NOT ${CMAKE_BUILD_TYPE} STREQUAL "Release")
   target_compile_definitions(${TARGETS_WM}
       PRIVATE
           _GLIBCXX_DEBUG)
endif()

target_compile_options(${TARGETS_WM}
    PRIVATE
        -Wall -Wextra -Wno-unused-parameter -Wno-comment)

set_target_properties(${TARGETS_WM}
    PROPERTIES
    # INTERPROCEDURAL_OPTIMIZATION ON

        CXX_EXTENSIONS OFF
        CXX_STANDARD 14
        CXX_STANDARD_REQUIRED ON

        C_EXTENSIONS OFF
        C_STANDARD 99
        C_STANDARD_REQUIRED ON

        LINK_FLAGS "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/../export.map")

if (LINK_LIBCXX)
   set_target_properties(${TARGETS_WM}
           PROPERTIES
           LINK_FLAGS "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/../export.map -lc++")
endif()

if (NOT ${SANITIZER_MODE} STREQUAL "none" AND NOT ${SANITIZER_MODE} STREQUAL "")
   target_compile_options(${TARGETS_WM}
      PRIVATE
         -fsanitize=${SANITIZER_MODE} -g -fno-omit-frame-pointer)
   set_target_properties(${TARGETS_WM}
      PROPERTIES
         LINK_FLAGS "-fsanitize=${SANITIZER_MODE} -g")
endif()

if(NOT EXISTS ${PROJECT_BINARY_DIR}/package)
   add_custom_command(TARGET ${TARGETS_WM} POST_BUILD
      COMMAND cp -rf ${PROJECT_SOURCE_DIR}/package ${PROJECT_BINARY_DIR}
   )
endif()

add_custom_command(TARGET ${TARGETS_WM} POST_BUILD
   COMMAND mkdir -p ${PROJECT_BINARY_DIR}/package/root/lib
   COMMAND cp -rf ${PROJECT_BINARY_DIR}/src/${TARGETS_WM}.so ${PROJECT_BINARY_DIR}/package/root/lib
   COMMAND mkdir -p ${PROJECT_BINARY_DIR}/package/root/etc
   COMMAND cp -f ${PROJECT_SOURCE_DIR}/layers.json ${PROJECT_BINARY_DIR}/package/root/etc
   COMMAND cp -f ${PROJECT_SOURCE_DIR}/src/db/old_roles.db ${PROJECT_BINARY_DIR}/package/root/etc
   COMMAND cp -f ${PROJECT_SOURCE_DIR}/src/db/areas.db ${PROJECT_BINARY_DIR}/package/root/etc
   COMMAND cp -f ${PROJECT_SOURCE_DIR}/conf/setting.json ${PROJECT_BINARY_DIR}/package/root/etc
)

add_custom_target(package DEPENDS ${PROJECT_BINARY_DIR}/package/root
   COMMAND wgtpkg-pack -f -o ${PROJECT_BINARY_DIR}/package/${TARGETS_WM}-2017.wgt ${PROJECT_BINARY_DIR}/package/root
)
