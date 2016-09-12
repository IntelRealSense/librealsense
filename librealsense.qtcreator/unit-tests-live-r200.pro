include(include.pri)

TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt
QMAKE_CXXFLAGS += -std=c++11 -g
QMAKE_CXXFLAGS += -D_FORTIFY_SOURCE=2 -fstack-protector-strong -Wformat -Wformat-security
QMAKE_CXXFLAGS += -z noexecstack -z relro -z now
QMAKE_CXXFLAGS_RELEASE *= -Ofast
PKGCONFIG += libusb-1.0
CONFIG += link_pkgconfig
INCLUDEPATH += ../include
LIBS += -pthread

SOURCES += ../unit-tests/unit-tests-live-r200.cpp \
    ../unit-tests/unit-tests-live-ds-common.cpp
SOURCES += ../unit-tests/unit-tests-live.cpp
SOURCES += ../unit-tests/unit-tests-common.h

LIBS += -L$$DESTDIR/ -lrealsense

PRE_TARGETDEPS += $$DESTDIR/librealsense.a

HEADERS += \
    ../unit-tests/unit-tests-live-ds-common.h
