include(LRS.pri)

TEMPLATE = lib
CONFIG += staticlib
CONFIG -= qt

INCLUDEPATH += ../include

CONFIG += link_pkgconfig
PKGCONFIG += libusb-1.0
LIBS += -pthread
QMAKE_CXXFLAGS += -std=c++11 -fPIC -pedantic -Wno-missing-field-initializers -Wno-switch

HEADERS += ../include/librealsense/* ../src/*.h ../src/libuvc/*.h
SOURCES += ../src/*.cpp ../src/verify.c ../src/libuvc/*.c
