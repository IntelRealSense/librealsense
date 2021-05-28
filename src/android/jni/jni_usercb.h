// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#ifndef LIBREALSENSE_JNI_USER_H
#define LIBREALSENSE_JNI_USER_H

#include <jni.h>
#include "../../../include/librealsense2/rs.h"
#include "../../../include/librealsense2/hpp/rs_frame.hpp"

// Java user frame callback data
struct rs_jni_cbdata
{
    JavaVM *jvm;           // Java VM interface
    jint version;          // Java VM version
    bool attached;         // if Java JVM is attached to current thread
    jobject jcb;           // Java user callback with OnFrame interface
    jclass frameclass;     // librealsense Java Frame class
};

typedef struct rs_jni_cbdata rs_jni_cbdata;

bool rs_jni_callback_init(JNIEnv *env, jobject jcb, rs_jni_cbdata* ud);
bool rs_jni_cb(rs2::frame f, rs_jni_cbdata* ud);
void rs_jni_cleanup(JNIEnv *env, rs_jni_cbdata* ud);

#endif
