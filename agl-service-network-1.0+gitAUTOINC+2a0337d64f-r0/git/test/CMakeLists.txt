###########################################
# build and install afb-client-demo
###########################################
PKG_CHECK_MODULES(libsystemd libsystemd>=222)
PKG_CHECK_MODULES(libafbwsc libafbwsc>=5.99)

ADD_EXECUTABLE(agl-service-network-ctl agl-service-network-ctl.c)

TARGET_LINK_LIBRARIES(agl-service-network-ctl
	${link_libraries}
	${libsystemd_LDFLAGS}
	${libafbwsc_LDFLAGS}
)
INSTALL(TARGETS agl-service-network-ctl
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

