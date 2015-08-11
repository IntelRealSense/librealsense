TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt
QMAKE_CXXFLAGS += -fPIC -std=c++11
PKGCONFIG += glfw3 gl
CONFIG += link_pkgconfig
INCLUDEPATH += ../../third_party/libuvc/include/ ../../include
LIBS += -pthread -ljpeg
PKGCONFIG += libusb-1.0

SOURCES += \
    ../../PointcloudViewer/PointcloudViewer/GfxUtil.cpp \
    ../../PointcloudViewer/PointcloudViewer/main.cpp

HEADERS += \
    ../../PointcloudViewer/PointcloudViewer/GfxUtil.h


unix:!macx: LIBS += -L$$OUT_PWD/../librealsense/ -llibrealsense

INCLUDEPATH += $$PWD/../librealsense
DEPENDPATH += $$PWD/../librealsense

unix:!macx: PRE_TARGETDEPS += $$OUT_PWD/../librealsense/liblibrealsense.a

unix:!macx: LIBS += -L$$OUT_PWD/../libuvc/ -llibuvc

INCLUDEPATH += $$PWD/../libuvc
DEPENDPATH += $$PWD/../libuvc

unix:!macx: PRE_TARGETDEPS += $$OUT_PWD/../libuvc/liblibuvc.a
