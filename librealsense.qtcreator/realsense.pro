include(include.pri)

TEMPLATE = lib
CONFIG += staticlib
CONFIG -= qt

INCLUDEPATH += ../include

CONFIG += link_pkgconfig
PKGCONFIG += libusb-1.0
LIBS += -pthread -L/usr/local/lib/motion/ -linfra -lmotionautoexposure -lmotionHAL -lmultirealsense -lslimAPI -ltbbmalloc -ltbb

QMAKE_CXXFLAGS += -std=c++11 -fPIC -pedantic -mssse3 -g
QMAKE_CXXFLAGS += -Wno-missing-field-initializers -Wno-switch -Wno-multichar
QMAKE_CXXFLAGS += -D_FORTIFY_SOURCE=2 -fstack-protector-strong -Wformat -Wformat-security
QMAKE_CXXFLAGS += -z noexecstack -z relro -z now
QMAKE_CXXFLAGS += -DRS_USE_V4L2_BACKEND
QMAKE_CXXFLAGS += -Wl,--no-as-needed
QMAKE_CXXFLAGS_RELEASE *= -Ofast

HEADERS += ../include/librealsense/* ../src/*.h
SOURCES += ../src/*.cpp ../src/verify.c

