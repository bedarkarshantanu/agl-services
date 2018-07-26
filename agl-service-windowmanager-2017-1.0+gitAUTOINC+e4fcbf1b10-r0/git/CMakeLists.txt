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

project(WindowManagerTMC)

set(PACKAGE_VERSION_MAJOR 0)
set(PACKAGE_VERSION_MINOR 0)
set(PACKAGE_VERSION_REVISION 1)
set(PACKAGE_VERSION "${PACKAGE_VERSION_MAJOR}.${PACKAGE_VERSION_MINOR}.${PACKAGE_VERSION_REVISION}")

find_package(PkgConfig REQUIRED)
include(GNUInstallDirs)

pkg_check_modules(WLC wayland-client>=1.11.0 REQUIRED)

macro(wlproto var_basename proto_xml_basename)
   if("${WLSCAN}" STREQUAL "")
      find_program(WLSCAN NAMES wayland-scanner)
   endif()

   if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/protocol)
      set(proto_dir "${CMAKE_CURRENT_SOURCE_DIR}/protocol")
   elseif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../protocol)
      set(proto_dir "${CMAKE_CURRENT_SOURCE_DIR}/../protocol")
   else()
      message(FATAL_ERROR "Could not find ${CMAKE_CURRENT_SOURCE_DIR}/protocol or ${CMAKE_CURRENT_SOURCE_DIR}/../protocol")
   endif()

   add_custom_command(
      OUTPUT
         ${proto_xml_basename}-protocol.c
         #${proto_xml_basename}-server-protocol.h
         ${proto_xml_basename}-client-protocol.h
      MAIN_DEPENDENCY ${proto_dir}/${proto_xml_basename}.xml
      COMMAND ${WLSCAN} code <          ${proto_dir}/${proto_xml_basename}.xml > ${proto_xml_basename}-protocol.c
      #COMMAND ${WLSCAN} server-header < ${proto_dir}/${proto_xml_basename}.xml > ${proto_xml_basename}-server-protocol.h
      COMMAND ${WLSCAN} client-header < ${proto_dir}/${proto_xml_basename}.xml > ${proto_xml_basename}-client-protocol.h
   )

   set(${var_basename}_PROTO ${CMAKE_CURRENT_BINARY_DIR}/${proto_xml_basename}-protocol.c)
   set(${var_basename}_CLIENT ${CMAKE_CURRENT_BINARY_DIR}/${proto_xml_basename}-client-protocol.h)
   #set(${var_basename}_SERVER ${CMAKE_CURRENT_BINARY_DIR}/${proto_xml_basename}-server-protocol.h)

   include_directories(${CMAKE_CURRENT_BINARY_DIR})
endmacro()

# Should modernize the following somehow...
set(ENABLE_DEBUG_OUTPUT OFF CACHE BOOL "Enable debug logging")
if(ENABLE_DEBUG_OUTPUT)
   add_definitions(-DDEBUG_OUTPUT)
else()
   remove_definitions(-DDEBUG_OUTPUT)
endif()

# Should modernize the following somehow...
set(ENABLE_SCOPE_TRACING OFF CACHE BOOL "Enable scope enter/leave messages for certain parts of the code.")
if(ENABLE_SCOPE_TRACING)
   add_definitions(-DSCOPE_TRACING)
else()
   remove_definitions(-DSCOPE_TRACING)
endif()

set(SANITIZER_MODE "none" CACHE STRING "Build using a specific sanitizer (e.g. 'address', 'thread', 'leak', 'undefined'), depends on compiler; default none")

set(LINK_LIBCXX OFF CACHE BOOL "Link against LLVMs libc++")

add_subdirectory(src)
