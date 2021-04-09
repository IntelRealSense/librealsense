/* License: Apache 2.0. See LICENSE file in root directory. */
/* Copyright(c) 2021 Intel Corporation. All Rights Reserved. */

#ifndef LIBREALSENSE_JNI_USER_H
#define LIBREALSENSE_JNI_USER_H

struct rs_jni_userdata
{
    JavaVM *jvm;
    jint version;
    bool attached;
    jobject jcb;
    jclass frameclass;
};

typedef struct rs_jni_userdata rs_jni_userdata;

#endif