include(LRS.pri)

TEMPLATE = lib
CONFIG += staticlib
CONFIG -= qt

INCLUDEPATH += ../include

CONFIG += link_pkgconfig
PKGCONFIG += libusb-1.0
LIBS += -pthread
QMAKE_CXXFLAGS += -std=c++11 -fpermissive -fPIC -Wno-missing-field-initializers

HEADERS += ../include/librealsense/* ../src/*.h ../src/libuvc/*.h \
    ../src/context.h
SOURCES += ../src/*.cpp ../src/verify.c ../src/libuvc/*.c \
    ../src/context.cpp
