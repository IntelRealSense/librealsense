#include <jni.h>
#include <error.h>
#include "../../../include/librealsense2/rs.h"

JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_StreamProfile_nGetProfile(JNIEnv *env, jclass type,
                                                                jlong handle, jobject params) {
    rs2_stream stream_type = RS2_STREAM_ANY;
    rs2_format format = RS2_FORMAT_ANY;
    int index = -1;
    int uniqueId = -1;
    int frameRate = -1;
    int e = 0;

    rs2_get_stream_profile_data(handle, &stream_type, &format, &index, &uniqueId, &frameRate, &e);


    jclass clazz = (*env)->GetObjectClass(env, params);

    jfieldID typeField = (*env)->GetFieldID(env, clazz, "type", "I");
    jfieldID formatField = (*env)->GetFieldID(env, clazz, "format", "I");
    jfieldID indexField = (*env)->GetFieldID(env, clazz, "index", "I");
    jfieldID uniqueIdField = (*env)->GetFieldID(env, clazz, "uniqueId", "I");
    jfieldID frameRateField = (*env)->GetFieldID(env, clazz, "frameRate", "I");

    (*env)->SetIntField(env, params, typeField, stream_type);
    (*env)->SetIntField(env, params, formatField, format);
    (*env)->SetIntField(env, params, indexField, index);
    (*env)->SetIntField(env, params, uniqueIdField, uniqueId);
    (*env)->SetIntField(env, params, frameRateField, frameRate);
}

JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_VideoStreamProfile_nGetResolution(JNIEnv *env, jclass type,
                                                                        jlong handle,
                                                                        jobject params) {
    int width = -1;
    int height = -1;
    int e = 0;

    rs2_get_video_stream_resolution(handle, &width, &height, &e);


    jclass clazz = (*env)->GetObjectClass(env, params);

    jfieldID widthField = (*env)->GetFieldID(env, clazz, "width", "I");
    jfieldID heightField = (*env)->GetFieldID(env, clazz, "height", "I");

    (*env)->SetIntField(env, params, widthField, width);
    (*env)->SetIntField(env, params, heightField, height);
}

JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_StreamProfile_nDelete(JNIEnv *env, jclass type,
                                                            jlong handle) {
    rs2_delete_stream_profile(handle);
}