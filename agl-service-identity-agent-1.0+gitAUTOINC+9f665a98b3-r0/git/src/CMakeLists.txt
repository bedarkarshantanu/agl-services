###########################################################################
# Copyright 2016, 2017 IoT.bzh
#
# author: José Bollo <jose.bollo@iot.bzh>
# author: Stéphane Desneux <stephane.desneux@iot.bzh>
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

PROJECT_TARGET_ADD(afb-identity-binding)

add_library(afb-identity-binding MODULE
	agl-identity-binding.c
	agl-forgerock.c
	aia-get.c
	authorization.c
	base64.c
	curl-wrap.c
	escape.c
	oidc-agent.c
)

pkg_check_modules(EXTRAS REQUIRED libcurl)

set_target_properties(afb-identity-binding PROPERTIES
        LABELS "BINDING"
	PREFIX ""
	COMPILE_FLAGS "${EXTRAS_CFLAGS} -DFOR_AFB_BINDING"
        LINK_FLAGS "${BINDINGS_LINK_FLAG}"
	LINK_LIBRARIES "${EXTRAS_LIBRARIES}"
        OUTPUT_NAME "${TARGET_NAME}"
)

add_custom_command(TARGET ${TARGET_NAME}
PRE_BUILD
COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/../package/htdocs
COMMAND cp -rv ${CMAKE_CURRENT_SOURCE_DIR}/../htdocs ${CMAKE_CURRENT_BINARY_DIR}/../package/
COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/../package/etc
COMMAND cp -rv ${CMAKE_CURRENT_SOURCE_DIR}/../etc ${CMAKE_CURRENT_BINARY_DIR}/../package/)
