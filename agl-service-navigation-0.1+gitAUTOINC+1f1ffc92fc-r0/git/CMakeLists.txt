cmake_minimum_required(VERSION 2.8.11)

INCLUDE(FindPkgConfig)

set(PROJECT_NAME "agl-service-navigation")
set(PROJECT_PRETTY_NAME "AFM binding for navigation service")
set(PROJECT_DESCRIPTION "Binding for AGL Navigation API")
set(PROJECT_INCLUDEDIR ${CMAKE_INSTALL_PREFIX}/include)
set(PROJECT_LIBDIR ${CMAKE_INSTALL_PREFIX}/lib)
set(PROJECT_API_VERSION 0.1.0)
set(PROJECT_URL "https://wiki.automotivelinux.org/eg-navi")

# Set a default build type if none was specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
endif()

project(navi_binder)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fPIC ")

set(CMAKE_INCLUDE_CURRENT_DIR ON)

pkg_check_modules(DBUSCXX REQUIRED dbus-c++-1)
pkg_check_modules(JSON REQUIRED json-c)
pkg_check_modules(AFBWSC REQUIRED libafbwsc)

include_directories( ${PROJECT_SOURCE_DIR}/libnavi/include ${PROJECT_SOURCE_DIR}/include ${DBUSCXX_INCLUDE_DIRS} ${JSON_INCLUDE_DIRS} ${AFBWSC_INCLUDE_DIRS} )

add_library( naviapi-agl SHARED libnavi/src/navicore.cpp libnavi/src/navicorelistener.cpp libnavi/src/BinderClient.cpp libnavi/src/JsonRequestGenerator.cpp libnavi/src/JsonResponseAnalyzer.cpp libnavi/src/RequestManage.cpp )
target_link_libraries( naviapi-agl ${JSON_LIBRARIES} ${AFBWSC_LIBRARIES} )
set_target_properties(naviapi-agl PROPERTIES VERSION 0.1.0 SOVERSION 0)

install(TARGETS naviapi-agl LIBRARY DESTINATION ${PROJECT_LIBDIR})
install(FILES ${PROJECT_SOURCE_DIR}/libnavi/include/libnavicore.hpp DESTINATION ${PROJECT_INCLUDEDIR})

add_library( NaviAPIService SHARED src/api.cpp src/analyze_request.cpp src/binder_reply.cpp src/genivi_request.cpp )

target_link_libraries( NaviAPIService ${DBUSCXX_LIBRARIES} ${JSON_LIBRARIES} ${AFBWSC_LIBRARIES} )

##########################################################################
# AGL binding
configure_file(config.xml.in config.xml)
set(BINARY_NAME "naviapi.wgt")

add_custom_command(
	OUTPUT ${BINARY_NAME}
	DEPENDS NaviAPIService
	COMMAND rm -rf package
	COMMAND mkdir -p package/root
	COMMAND mkdir -p package/root/lib
	COMMAND mv config.xml package/root/
	COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/navigation.png package/root/icon.png
	COMMAND mv libNaviAPIService.so package/root/lib
	COMMAND wgtpkg-pack -f -o package/${BINARY_NAME} package/root
)
add_custom_target(widget ALL DEPENDS ${BINARY_NAME})

#generate configure file
configure_file(libnaviapi-agl.pc.in libnaviapi-agl.pc @ONLY)
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/libnaviapi-agl.pc
  DESTINATION
  ${CMAKE_INSTALL_PREFIX}/lib/pkgconfig)
