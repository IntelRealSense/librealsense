#include <jni.h>
#include <error.h>
#include "../../../include/librealsense2/rs.h"

JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_VideoStreamProfile_nGetResolution(JNIEnv *env,
                                                                        jobject instance,
                                                                        jlong handle, jint width,
                                                                        jint height,
                                                                        jint error) {
    rs2_get_video_stream_resolution(handle, &width, &height, &error);
}

JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_StreamProfile_nGetProfile(JNIEnv *env, jobject instance,
                                                                jlong handle, jobject params) {
    rs2_stream type = RS2_STREAM_ANY;
    rs2_format format = RS2_FORMAT_ANY;
    int index = -1;
    int uniqueId = -1;
    int frameRate = -1;
    int e = 0;

    rs2_get_stream_profile_data(handle, &type, &format, &index, &uniqueId, &frameRate, &e);


    jclass clazz = (*env)->GetObjectClass(env, params);

    jfieldID typeField = (*env)->GetFieldID(env, clazz, "type", "I");
    jfieldID formatField = (*env)->GetFieldID(env, clazz, "format", "I");
    jfieldID indexField = (*env)->GetFieldID(env, clazz, "index", "I");
    jfieldID uniqueIdField = (*env)->GetFieldID(env, clazz, "uniqueId", "I");
    jfieldID frameRateField = (*env)->GetFieldID(env, clazz, "frameRate", "I");
    jfieldID errorField = (*env)->GetFieldID(env, clazz, "error", "I");

    (*env)->SetIntField(env, params, typeField, type);
    (*env)->SetIntField(env, params, formatField, format);
    (*env)->SetIntField(env, params, indexField, index);
    (*env)->SetIntField(env, params, uniqueIdField, uniqueId);
    (*env)->SetIntField(env, params, frameRateField, frameRate);
    (*env)->SetIntField(env, params, errorField, e);
}