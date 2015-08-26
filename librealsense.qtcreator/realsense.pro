include(LRS.pri)

TEMPLATE = lib
CONFIG += staticlib
CONFIG -= qt

INCLUDEPATH += ../include

CONFIG += link_pkgconfig
PKGCONFIG += libusb-1.0
LIBS += -pthread
QMAKE_CXXFLAGS += -std=c++11 -fpermissive -fPIC -Wno-missing-field-initializers

HEADERS += ../include/librealsense/* ../src/*.h ../src/F200/*.h ../src/R200/*.h
HEADERS += ../src/libuvc/*.h

SOURCES += ../src/verify.c ../src/*.cpp ../src/F200/*.cpp ../src/R200/*.cpp
SOURCES += ../src/libuvc/*.c
