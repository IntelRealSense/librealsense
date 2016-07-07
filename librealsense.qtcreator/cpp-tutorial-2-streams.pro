include(include.pri)

TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt
QMAKE_CXXFLAGS += -std=c++11
QMAKE_CXXFLAGS += -O2 -D_FORTIFY_SOURCE=2 -fstack-protector-strong -Wformat -Wformat-security
QMAKE_CXXFLAGS += -z noexecstack -z relro -z now
PKGCONFIG += glfw3 gl libusb-1.0
CONFIG += link_pkgconfig
INCLUDEPATH += ../include
LIBS += -pthread

SOURCES += ../examples/cpp-tutorial-2-streams.cpp

LIBS += -L$$DESTDIR/ -lrealsense

PRE_TARGETDEPS += $$DESTDIR/librealsense.a

