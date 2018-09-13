SAVED_LOCAL_PATH := $(LOCAL_PATH)
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

#------------------------------------------------------------------------------
# CFLAGS
LOCAL_CFLAGS	+=  -DANDROID -w -fstack-protector -fPIE -fPIC -pie -Wformat -Wformat-security
LOCAL_CPPFLAGS	+=  -DANDROID -w -fstack-protector -fPIE -fPIC -pie -Wformat -Wformat-security

ifneq ($(filter userdebug eng tests, $(TARGET_BUILD_VARIANT)),)
LOCAL_CFLAGS	+= -g -O0
LOCAL_CPPFLAGS	+= -g -O0
endif

# adding c++11 support
LOCAL_CPPFLAGS	+= -std=c++11 -fexceptions
include external/libcxx/libcxx.mk

#------------------------------------------------------------------------------
# INCLUDES
LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../include \
	$(SYSTEM_INCLUDE_PATHS) \
	$(FRAMEWORK_INCLUDE_PATHS) \

#------------------------------------------------------------------------------
# SRC
LOCAL_SRC_FILES := \
	Log.c \
	Trace.c \
	Dispatcher.cpp \
	Fsm.cpp \
	FormatConverter.cpp

#------------------------------------------------------------------------------
# LINKER
LOCAL_LDFLAGS += \
	-z noexecstack \

LOCAL_STATIC_LIBRARIES += \

LOCAL_SHARED_LIBRARIES += \
	libcutils \
	libutils \

LOCAL_LDLIBS += \
	-llog \

#------------------------------------------------------------------------------
# BUILD client API
LOCAL_MODULE_TAGS := optional
LOCAL_MULTILIB    := 64
LOCAL_MODULE      := libmminfra
include $(BUILD_SHARED_LIBRARY)

LOCAL_PATH := $(SAVED_LOCAL_PATH)
