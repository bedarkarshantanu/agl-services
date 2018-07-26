###########################################################################
# Copyright 2015, 2016, 2017 IoT.bzh
#
# author: Fulup Ar Foll <rfulup@iot.bzh>
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
###########################################################################

# This is a CMakeLists.txt file meant to be included as submodule into an AGL
# app using app-templates subdmodules

# Add target to project dependency list
PROJECT_TARGET_ADD(afb-helpers)
set(CMAKE_AUTOMOC ON)

	set(AFB_HELPERS_SRCS wrap-json.c filescan-utils.c escape.c)

	option(AFB_HELPERS_QTWSCLIENT "Enable the Qt's websocket client to Application Framework Binders" OFF)
	find_package(CURL)

	if (AFB_HELPERS_QTWSCLIENT)
		message(STATUS "Qt's WebSocket AFB Client: Enabled!")
		set(AFB_HELPERS_SRCS ${AFB_HELPERS_SRCS} qafbwebsocketclient.cpp qafbwebsocketclient.h)
		find_package(Qt5WebSockets REQUIRED)
	else()
		message(STATUS "Qt's WebSocket AFB Client: Disabled!")
	endif()

	if (CURL_FOUND)
		message(STATUS "CURL wrapping helpers: Enabled!")
		set(AFB_HELPERS_SRCS ${AFB_HELPERS_SRCS} curl-wrap.c)
	else()
		message(STATUS "CURL wrapping helpers: Disabled!")
	endif()

	# Define targets
	ADD_LIBRARY(${TARGET_NAME} STATIC ${AFB_HELPERS_SRCS})

	if (AFB_HELPERS_QTWSCLIENT)
		target_link_libraries(${TARGET_NAME} Qt5::WebSockets)
		qt5_use_modules(${TARGET_NAME} WebSockets)
	endif()

	if (CURL_FOUND)
		target_link_libraries(${TARGET_NAME} curl)
	endif()

	# Library properties
	SET_TARGET_PROPERTIES(${TARGET_NAME} PROPERTIES
		OUTPUT_NAME ${TARGET_NAME}
	)

	# Define target includes
	TARGET_INCLUDE_DIRECTORIES(${TARGET_NAME}
		PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}
	)

