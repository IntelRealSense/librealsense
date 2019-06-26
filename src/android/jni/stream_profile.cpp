// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#include <jni.h>
#include "error.h"
#include "../../../include/librealsense2/rs.h"

extern "C" JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_StreamProfile_nGetProfile(JNIEnv *env, jclass type,
                                                                jlong handle, jobject params) {
    rs2_stream stream_type = RS2_STREAM_ANY;
    rs2_format format = RS2_FORMAT_ANY;
    int index = -1;
    int uniqueId = -1;
    int frameRate = -1;
    rs2_error *e = NULL;

    rs2_get_stream_profile_data((const rs2_stream_profile *) handle, &stream_type, &format, &index, &uniqueId, &frameRate, &e);
    handle_error(env, e);

    jclass clazz = env->GetObjectClass(params);

    jfieldID typeField = env->GetFieldID(clazz, "type", "I");
    jfieldID formatField = env->GetFieldID(clazz, "format", "I");
    jfieldID indexField = env->GetFieldID(clazz, "index", "I");
    jfieldID uniqueIdField = env->GetFieldID(clazz, "uniqueId", "I");
    jfieldID frameRateField = env->GetFieldID(clazz, "frameRate", "I");

    env->SetIntField(params, typeField, stream_type);
    env->SetIntField(params, formatField, format);
    env->SetIntField(params, indexField, index);
    env->SetIntField(params, uniqueIdField, uniqueId);
    env->SetIntField(params, frameRateField, frameRate);
}

extern "C" JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_VideoStreamProfile_nGetIntrinsics(JNIEnv *env, jclass type,
                                                                        jlong handle,
                                                                        jobject params) {
    rs2_intrinsics intr;
    intr={0};
    rs2_error *e = NULL;

    rs2_get_video_stream_intrinsics((const rs2_stream_profile *) handle,&intr,&e);
    handle_error(env, e);

    jclass clazz = env->GetObjectClass(params);

    jfieldID widthField = env->GetFieldID(clazz, "width", "I");
    jfieldID heightField = env->GetFieldID(clazz, "height", "I");
    jfieldID fxField = env->GetFieldID(clazz, "fx", "F");
    jfieldID fyField = env->GetFieldID(clazz, "fy", "F");
    jfieldID ppxField = env->GetFieldID(clazz, "ppx", "F");
    jfieldID ppyField = env->GetFieldID(clazz, "ppy", "F");


    env->SetIntField(params, widthField, intr.width);
    env->SetIntField(params, heightField, intr.height);
    env->SetFloatField(params,fxField,intr.fx);
    env->SetFloatField(params,fyField,intr.fy);
    env->SetFloatField(params,ppxField,intr.ppx);
    env->SetFloatField(params,ppyField,intr.ppy);

}

extern "C" JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_StreamProfile_nDelete(JNIEnv *env, jclass type,
                                                            jlong handle) {
    rs2_delete_stream_profile((rs2_stream_profile *) handle);
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_intel_realsense_librealsense_StreamProfile_nIsProfileExtendableTo(JNIEnv *env, jclass type,
                                                                           jlong handle,
                                                                           jint extension) {
    rs2_error *e = NULL;
    int rv = rs2_stream_profile_is(reinterpret_cast<const rs2_stream_profile *>(handle),
                                   static_cast<rs2_extension>(extension), &e);
    handle_error(env, e);
    return rv;
}
