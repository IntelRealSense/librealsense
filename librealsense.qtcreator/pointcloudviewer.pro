include(LRS.pri)

TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt
CONFIG += link_pkgconfig

QMAKE_CXXFLAGS += -std=c++11
PKGCONFIG += glfw3 gl libusb-1.0 glew
INCLUDEPATH += ../third_party/libuvc/include/ ../include
LIBS += -pthread -ljpeg

SOURCES += \
    ../PointcloudViewer/PointcloudViewer/GfxUtil.cpp \
    ../PointcloudViewer/PointcloudViewer/main.cpp

HEADERS += \
    ../PointcloudViewer/PointcloudViewer/GfxUtil.h \
    ../include/librealsense/rs.h \
    ../include/librealsense/rs.hpp


LIBS += -L$$DESTDIR/ -lrealsense -luvc

PRE_TARGETDEPS += $$DESTDIR/librealsense.a
PRE_TARGETDEPS += $$DESTDIR/libuvc.a
