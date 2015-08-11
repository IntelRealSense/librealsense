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

SOURCES += ../PointcloudViewer/PointcloudViewer/*.cpp
HEADERS += ../PointcloudViewer/PointcloudViewer/*.h

LIBS += -L$$DESTDIR/ -lrealsense -luvc

PRE_TARGETDEPS += $$DESTDIR/librealsense.a
PRE_TARGETDEPS += $$DESTDIR/libuvc.a
