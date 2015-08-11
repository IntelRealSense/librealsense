include(LRS.pri)

TEMPLATE = lib
CONFIG += staticlib
CONFIG -= qt

INCLUDEPATH += ../third_party/libuvc/include/ ../include

CONFIG += link_pkgconfig
PKGCONFIG += libusb-1.0
LIBS += -pthread -ljpeg
QMAKE_CXXFLAGS += -std=c++11 -fpermissive -fPIC

HEADERS += \
    ../include/librealsense/* \
    ../src/F200/*.h \
    ../src/R200/*.h \
    ../src/*.h

SOURCES += \
    ../src/F200/*.cpp \
    ../src/R200/*.cpp \
    ../src/*.cpp

PRE_TARGETDEPS += $$DESTDIR/libuvc.a
