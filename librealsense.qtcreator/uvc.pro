include(LRS.pri)

TEMPLATE = lib
CONFIG += staticlib
CONFIG -= qt

INCLUDEPATH += ../third_party/libuvc/include/
SOURCES += \
    ../third_party/libuvc/src/ctrl.c \
    ../third_party/libuvc/src/device.c \
    ../third_party/libuvc/src/diag.c \
    ../third_party/libuvc/src/frame.c \
    ../third_party/libuvc/src/init.c \
    ../third_party/libuvc/src/misc.c \
    ../third_party/libuvc/src/stream.c

HEADERS += \
    ../third_party/libuvc/include/libuvc/libuvc.h \
    ../third_party/libuvc/include/libuvc/libuvc_config.h \
    ../third_party/libuvc/include/libuvc/libuvc_internal.h \
    ../third_party/libuvc/include/utlist.h

CONFIG += link_pkgconfig
PKGCONFIG += libusb-1.0
LIBS += -pthread -ljpeg
QMAKE_CFLAGS += -fPIC -DUVC_DEBUGGING
