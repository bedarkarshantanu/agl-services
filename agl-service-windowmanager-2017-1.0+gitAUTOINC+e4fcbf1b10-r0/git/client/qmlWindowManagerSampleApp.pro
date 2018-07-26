TEMPLATE = app
QT += qml quick
CONFIG += c++11

HEADERS += communication.h qlibwindowmanager.h

SOURCES += main.cpp \
           communication.cpp qlibwindowmanager.cpp
RESOURCES += sample.qrc

INCLUDEPATH += $$[QT_SYSROOT]/usr/include/afb

LIBS += -lwindowmanager
#LIBS += -lsystemd
LIBS += -lafbwsc
#LIBS += -ljson-c

target.path = /usr/bin/WindowManagerSampleApp

INSTALLS += target
