include(LRS.pri)

TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt
QMAKE_CXXFLAGS += -std=c++11
PKGCONFIG += glfw3 gl glu libusb-1.0
CONFIG += link_pkgconfig
INCLUDEPATH += ../third_party/libuvc/include/ ../include ../third_party/stb
LIBS += -pthread -ljpeg

SOURCES += ../examples/pointcloud/pointcloud.cpp


LIBS += -L$$DESTDIR/ -lrealsense -luvc

PRE_TARGETDEPS += $$DESTDIR/librealsense.a
PRE_TARGETDEPS += $$DESTDIR/libuvc.a

