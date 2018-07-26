TARGET = agl-bluetooth-binding

HEADERS = bluetooth-api.h bluetooth-manager.h bluetooth-agent.h lib_agent.h ofono-client.h lib_ofono.h lib_ofono_modem.h bluez-client.h lib_bluez.h
SOURCES = bluetooth-api.c bluetooth-manager.c bluetooth-agent.c lib_agent.c ofono-client.c lib_ofono.c lib_ofono_modem.c bluez-client.c lib_bluez.c 

LIBS += -Wl,--version-script=$$PWD/export.map

CONFIG += link_pkgconfig
PKGCONFIG += json-c afb-daemon glib-2.0 gio-2.0 gobject-2.0 gio-unix-2.0 zlib

include(binding.pri)
