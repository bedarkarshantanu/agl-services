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

cmake_minimum_required(VERSION 2.8)

set(TARGETS_HSBINDER homescreen-service)

INCLUDE(FindThreads)
FIND_PACKAGE(Threads)

pkg_check_modules(hs_binding_depends afb-daemon glib-2.0 gio-2.0 gio-unix-2.0 json-c)
set(binding_hs_sources
  homescreen.c
  hs-helper.c)

link_libraries(-Wl,--as-needed -Wl,--gc-sections -Wl,--no-undefined)
include_directories(${PROJECT_SOURCE_DIR}/include)
add_library(${TARGETS_HSBINDER} MODULE ${binding_hs_sources})

target_compile_options(${TARGETS_HSBINDER} PRIVATE ${hs_binding_depends_CFLAGS})
if(DEFINED DEBUGMODE)
 target_compile_options(${TARGETS_HSBINDER} PRIVATE -g -O0)
else(DEFINED DEBUGMODE)
 target_compile_options(${TARGETS_HSBINDER} PRIVATE -g -O2)
endif(DEFINED DEBUGMODE)

target_include_directories(${TARGETS_HSBINDER} PRIVATE ${hs_binding_depends_INCLUDE_DIRS})
target_link_libraries(${TARGETS_HSBINDER} ${CMAKE_THREAD_LIBS_INIT} ${link_libraries} ${hs_binding_depends_LIBRARIES})

# Binder exposes a unique public entry point

set_target_properties(${TARGETS_HSBINDER} PROPERTIES
    PREFIX ""
    LINK_FLAGS "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/export.map"
    )

# installation directory
#INSTALL(TARGETS ${TARGETS_HSBINDER}
#    LIBRARY DESTINATION ${binding_install_dir})

if(NOT EXISTS ${PROJECT_BINARY_DIR}/package)
    add_custom_command(TARGET ${TARGETS_HSBINDER} POST_BUILD
        COMMAND cp -rf ${PROJECT_SOURCE_DIR}/package ${PROJECT_BINARY_DIR}
    )
endif()

add_custom_command(TARGET ${TARGETS_HSBINDER} POST_BUILD
    COMMAND mkdir -p ${PROJECT_BINARY_DIR}/package/root/lib
    COMMAND cp -rf ${PROJECT_BINARY_DIR}/src/${TARGETS_HSBINDER}.so ${PROJECT_BINARY_DIR}/package/root/lib
)

add_custom_target(package DEPENDS ${PROJECT_BINARY_DIR}/package/root
    COMMAND wgtpkg-pack -f -o ${PROJECT_BINARY_DIR}/package/${TARGETS_HSBINDER}-2017.wgt ${PROJECT_BINARY_DIR}/package/root
)
