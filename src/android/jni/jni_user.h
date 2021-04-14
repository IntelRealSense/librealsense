/* License: Apache 2.0. See LICENSE file in root directory. */
/* Copyright(c) 2021 Intel Corporation. All Rights Reserved. */

#ifndef LIBREALSENSE_JNI_USER_H
#define LIBREALSENSE_JNI_USER_H

#include <jni.h>
#include "../../../include/librealsense2/rs.h"
#include "../../../include/librealsense2/hpp/rs_frame.hpp"

struct rs_jni_userdata
{
    JavaVM *jvm;           // Java VM interface
    jint version;          // Java VM version
    bool attached;         // if Java JVM is attached to current thread
    jobject jcb;           // java user callback with OnFrame interface
    jclass frameclass;     // librealsense java Frame class
};

typedef struct rs_jni_userdata rs_jni_userdata;

bool rs_jni_callback_init(JNIEnv *env, jobject jcb, rs_jni_userdata* ud);
bool rs_jni_cb(rs2::frame f, rs_jni_userdata* ud);

#endif