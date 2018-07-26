###########################################################################
# Copyright 2015, 2016, 2017 IoT.bzh
# Copyright (C) 2018 Konsulko Group
#
# author: Fulup Ar Foll <fulup@iot.bzh>
# contrib: Romain Forlot <romain.forlot@iot.bzh>
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

# Add target to project dependency list
PROJECT_TARGET_ADD(radio-binding)

	add_definitions(-DAFB_BINDING_VERSION=2)

	# Define project Targets
	set(radio_SOURCES
		radio-binding.c
		radio_impl_kingfisher.c
		radio_impl_rtlsdr.c)
	if(HAVE_4A_FRAMEWORK)
		add_definitions(-DHAVE_4A_FRAMEWORK)
	endif()

	PKG_CHECK_MODULES(SOUND REQUIRED gstreamer-1.0)

	add_library(${TARGET_NAME} MODULE ${radio_SOURCES})

	# Binder exposes a unique public entry point
	SET_TARGET_PROPERTIES(${TARGET_NAME} PROPERTIES
		PREFIX "libafm-"
		LABELS "BINDING"
		LINK_FLAGS ${BINDINGS_LINK_FLAG}
		OUTPUT_NAME ${TARGET_NAME}
	)

	# Library dependencies (include updates automatically)
	TARGET_COMPILE_OPTIONS(${TARGET_NAME} PUBLIC ${SOUND_CFLAGS})
	TARGET_LINK_LIBRARIES(${TARGET_NAME} ${SOUND_LIBRARIES} ${link_libraries})

# Add helper program target
PROJECT_TARGET_ADD(rtl_fm_helper)

	# Define project targets
	set(helper_SOURCES
		${TARGET_NAME}.c
		rtl_fm.c
		convenience/convenience.c)
	if(HAVE_4A_FRAMEWORK)
		set(helper_SOURCES ${helper_SOURCES} radio_output_gstreamer.c)
		PKG_CHECK_MODULES(helper_SOUND REQUIRED gstreamer-1.0)
	else()
		set(helper_SOURCES ${helper_SOURCES} radio_output_pulse.c)
		PKG_CHECK_MODULES(helper_SOUND REQUIRED libpulse-simple)
	endif()

	PKG_CHECK_MODULES(helper_RTLSDR REQUIRED librtlsdr)
	PKG_CHECK_MODULES(helper_LIBUSB REQUIRED libusb-1.0)

	add_executable(${TARGET_NAME} ${helper_SOURCES})

	SET_TARGET_PROPERTIES(${TARGET_NAME} PROPERTIES
		LABELS "EXECUTABLE"
		OUTPUT_NAME ${TARGET_NAME}
	)

	TARGET_COMPILE_OPTIONS(${TARGET_NAME} PUBLIC ${helper_SOUND_CFLAGS})
	TARGET_LINK_LIBRARIES(${TARGET_NAME}
		${helper_RTLSDR_LIBRARIES} ${helper_LIBUSB_LIBRARIES} ${helper_SOUND_LIBRARIES} ${link_libraries} m)
