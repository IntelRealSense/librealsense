// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#include <jni.h>
#include "error.h"

#include "../../../include/librealsense2/rs.h"

extern "C" JNIEXPORT jboolean JNICALL
Java_com_intel_realsense_librealsense_Options_nSupports(JNIEnv *env, jclass type, jlong handle, jint option) {
    rs2_error* e = NULL;
    auto rv = rs2_supports_option((const rs2_options *) handle, (rs2_option) option, &e);
    handle_error(env, e);
    return rv > 0;
}

extern "C" JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_Options_nSetValue(JNIEnv *env, jclass type, jlong handle,
                                                        jint option, jfloat value) {
    rs2_error* e = NULL;
    rs2_set_option(reinterpret_cast<const rs2_options *>(handle), static_cast<rs2_option>(option), value, &e);
    handle_error(env, e);
}

extern "C" JNIEXPORT jfloat JNICALL
Java_com_intel_realsense_librealsense_Options_nGetValue(JNIEnv *env, jclass type, jlong handle,
                                                        jint option) {
    rs2_error* e = NULL;
    float rv = rs2_get_option(reinterpret_cast<const rs2_options *>(handle),
                              static_cast<rs2_option>(option), &e);
    handle_error(env, e);
    return rv;
}

extern "C" JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_Options_nGetRange(JNIEnv *env, jclass type, jlong handle,
                                                        jint option, jobject outParams) {
    float min = -1;
    float max = -1;
    float step = -1;
    float def = -1;
    rs2_error *e = NULL;
    jclass clazz = env->GetObjectClass(outParams);

    rs2_get_option_range(reinterpret_cast<const rs2_options *>(handle),
                         static_cast<rs2_option>(option), &min, &max, &step, &def, &e);
    handle_error(env, e);

    if(e)
        return;

    jfieldID minField = env->GetFieldID(clazz, "min", "F");
    jfieldID maxField = env->GetFieldID(clazz, "max", "F");
    jfieldID stepField = env->GetFieldID(clazz, "step", "F");
    jfieldID defField = env->GetFieldID(clazz, "def", "F");

    env->SetFloatField(outParams, minField, min);
    env->SetFloatField(outParams, maxField, max);
    env->SetFloatField(outParams, stepField, step);
    env->SetFloatField(outParams, defField, def);
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_intel_realsense_librealsense_Options_nIsReadOnly(JNIEnv *env, jclass type, jlong handle,
                                                          jint option) {
    rs2_error* e = NULL;
    int rv = rs2_is_option_read_only(reinterpret_cast<const rs2_options *>(handle),
                                     static_cast<rs2_option>(option), &e);
    handle_error(env, e);
    return rv > 0;
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_intel_realsense_librealsense_Options_nGetDescription(JNIEnv *env, jclass type,
                                                              jlong handle, jint option) {
    rs2_error* e = NULL;
    const char *rv = rs2_get_option_description(reinterpret_cast<const rs2_options *>(handle),
                                                static_cast<rs2_option>(option), &e);
    handle_error(env, e);
    return env->NewStringUTF(rv);
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_intel_realsense_librealsense_Options_nGetValueDescription(JNIEnv *env, jclass type,
                                                                   jlong handle, jint option,
                                                                   jfloat value) {
    rs2_error* e = NULL;
    const char *rv = rs2_get_option_value_description(reinterpret_cast<const rs2_options *>(handle),
                                                      static_cast<rs2_option>(option), value, &e);
    handle_error(env, e);
    return env->NewStringUTF(rv);
}
