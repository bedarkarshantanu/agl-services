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

###########################################################################

link_libraries(-Wl,--as-needed -Wl,--gc-sections)

add_compile_options(-Wall -Wextra -Wconversion)
add_compile_options(-Wno-unused-parameter) # frankly not using a parameter does it care?
add_compile_options(-Werror=maybe-uninitialized)
add_compile_options(-Werror=implicit-function-declaration)
add_compile_options(-ffunction-sections -fdata-sections)
add_compile_options(-Wl,--as-needed -Wl,--gc-sections)
add_compile_options(-fPIC)

set(CMAKE_C_FLAGS_PROFILING    "-g -O0 -pg -Wp,-U_FORTIFY_SOURCE")
set(CMAKE_C_FLAGS_DEBUG        "-g -O0 -ggdb -Wp,-U_FORTIFY_SOURCE")
set(CMAKE_C_FLAGS_RELEASE      "-g -O2")
set(CMAKE_C_FLAGS_CCOV         "-g -O2 --coverage")
set(LIBRARY_OUTPUT_PATH		   ${PROJECT_BINARY_DIR}/package/root/lib)

###########################################################################

include(FindPkgConfig)

#*<NESSARY>  pkg_check_modules(EXTRAS REQUIRED json-c libsystemd afb-daemon)
#pkg_check_modules(EXTRAS REQUIRED json-c)
add_compile_options(${EXTRAS_CFLAGS})
include_directories(${EXTRAS_INCLUDE_DIRS})
link_libraries(${EXTRAS_LIBRARIES})

message(STATUS "${EXTRAS_CFLAGS}")
message(STATUS "${EXTRAS_INCLUDE_DIRS}")
message(STATUS "${EXTRAS_LIBRARIES}")
message(STATUS "${EXTRAS_LIBRARY_DIRS}")

pkg_check_modules(GIO_UNIX REQUIRED gio-unix-2.0)
include_directories(${GIO_UNIX_INCLUDE_DIRS})
link_libraries(${GIO_UNIX_LIBRARIES})
message(STATUS "${GIO_UNIX_INCLUDE_DIRS}")

#set(VAR "")
#get_target_property(VAR target INCLUDE_DIRECTORIES)
#message(STATUS "${VAR}")

###########################################################################
# the binding for afb

message(STATUS "Creation af-steering-wheel-binding for AFB-DAEMON")

###############################################################
#pkg_get_variable(afb_binding_install_dir afb-daemon binding_install_dir)

#*<NESSARY>
#* execute_process(
#*	COMMAND pkg-config --variable binding_install_dir afb-daemon
#*	OUTPUT_VARIABLE afb_binding_install_dir OUTPUT_STRIP_TRAILING_WHITESPACE
#* )

###############################################################
add_library(af-steering-wheel-binding MODULE af-steering-wheel-binding.c bind_event.c steering_wheel_json.c prop_info.c prop_search.c js_signal_event.c js_raw.c)
set_target_properties(af-steering-wheel-binding PROPERTIES
	PREFIX ""
	LINK_FLAGS "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/export.map"
)

add_custom_command(TARGET af-steering-wheel-binding POST_BUILD
		COMMAND cp -f ${CMAKE_CURRENT_SOURCE_DIR}/config.xml ${CMAKE_CURRENT_SOURCE_DIR}/steering_wheel.json ${CMAKE_CURRENT_SOURCE_DIR}/icon.png ${PROJECT_BINARY_DIR}/package/root
		COMMAND cp -f ${CMAKE_CURRENT_SOURCE_DIR}/steering_wheel_map.json ${CMAKE_CURRENT_SOURCE_DIR}/gear_shift_para.json ${PROJECT_BINARY_DIR}/package/root)

add_custom_target(package DEPENDS ${PROJECT_BINARY_DIR}/package/root
		COMMAND wgtpkg-pack -f -o ${PROJECT_BINARY_DIR}/package/${PROJECT_NAME}.wgt ${PROJECT_BINARY_DIR}/package/root)

#*<NESSARY>  install(TARGETS af-canivi-binding LIBRARY DESTINATION ${afb_binding_install_dir})
# <OR following packaging>
##############################################################
# WGT packaging

#message(STATUS "Creation of af-canivi.wgt package for AppFW")
##############################################################
#configure_file(config.xml.in config.xml)
#add_custom_command(
#    OUTPUT af-canivi.wgt
#    COMMAND rm -rf package
#    COMMAND mkdir -p package/libs package/htdocs
#    COMMAND cp config.xml package/
#    COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_ICON} package/icon.png
#    COMMAND cp af-canivi-binding.so package
#    COMMAND wgtpkg-pack -f -o af-canivi.wgt package
#)
#add_custom_target(widget ALL DEPENDS af-canivi.wgt)

