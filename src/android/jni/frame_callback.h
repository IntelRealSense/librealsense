// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#ifndef LIBREALSENSE_JNI_FRAME_CALLBACK_H
#define LIBREALSENSE_JNI_FRAME_CALLBACK_H

#include <jni.h>
#include "../../../include/librealsense2/rs.h"
#include "../../../include/librealsense2/hpp/rs_frame.hpp"

// Java user frame callback data
struct frame_callback_data
{
    JavaVM *jvm;           // Java VM interface
    jint version;          // Java VM version
    bool attached;         // if Java JVM is attached to current thread
    jobject frame_cb;      // Java user frame callback with OnFrame interface
    jclass frameclass;     // librealsense Java Frame class
};

typedef struct frame_callback_data  frame_callback_data;

bool rs_jni_callback_init(JNIEnv *env, jobject jcb, frame_callback_data* ud);
bool rs_jni_cb(rs2::frame f, frame_callback_data* ud);
void rs_jni_cleanup(JNIEnv *env, frame_callback_data* ud);

#endif
