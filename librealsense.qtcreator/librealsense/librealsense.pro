TEMPLATE = lib
CONFIG += staticlib
CONFIG -= qt

INCLUDEPATH += ../../third_party/libuvc/include/ ../../include


CONFIG += link_pkgconfig
PKGCONFIG += libusb-1.0
LIBS += -pthread -ljpeg
QMAKE_CXXFLAGS += -fPIC -std=c++11

HEADERS += \
    ../../include/librealsense/F200/CalibParams.h \
    ../../include/librealsense/F200/F200.h \
    ../../include/librealsense/F200/Projection.h \
    ../../include/librealsense/F200/XU.h \
    ../../include/librealsense/R200/CalibIO.h \
    ../../include/librealsense/R200/CalibParams.h \
    ../../include/librealsense/R200/CameraHeader.h \
    ../../include/librealsense/R200/R200.h \
    ../../include/librealsense/R200/SPI.h \
    ../../include/librealsense/R200/XU.h \
    ../../include/librealsense/CameraContext.h \
    ../../include/librealsense/Common.h \
    ../../include/librealsense/UVCCamera.h

SOURCES += \
    ../../src/F200/F200.cpp \
    ../../src/R200/R200.cpp \
    ../../src/R200/SPI.cpp \
    ../../src/R200/XU.cpp \
    ../../src/CameraContext.cpp \
    ../../src/UVCCamera.cpp


INCLUDEPATH += $$PWD/../libuvc
DEPENDPATH += $$PWD/../libuvc

PRE_TARGETDEPS += $$OUT_PWD/../libuvc/liblibuvc.a
