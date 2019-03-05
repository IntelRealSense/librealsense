#include <jni.h>
#include "error.h"

#include "../../../include/librealsense2/rs.h"

JNIEXPORT jboolean JNICALL
Java_com_intel_realsense_librealsense_Options_nSupports(JNIEnv *env, jclass type, jlong handle, jint option) {
    rs2_error* e = NULL;
    auto rv = rs2_supports_option((const rs2_options *) handle, (rs2_option) option, &e);
    handle_error(env, e);
    return rv > 0;
}

JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_Options_nSetValue(JNIEnv *env, jclass type, jlong handle,
                                                        jint option, jfloat value) {
    rs2_error* e = NULL;
    rs2_set_option((const rs2_options *) handle, (rs2_option) option, value, &e);
    handle_error(env, e);
}

JNIEXPORT jfloat JNICALL
Java_com_intel_realsense_librealsense_Options_nGetValue(JNIEnv *env, jclass type, jlong handle,
                                                        jint option) {
    rs2_error* e = NULL;
    float rv = rs2_get_option((const rs2_options *) handle, (rs2_option) option, &e);
    handle_error(env, e);
    return rv;
}

JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_Options_nGetRange(JNIEnv *env, jclass type, jlong handle,
                                                        jint option, jobject outParams) {
    float min = -1;
    float max = -1;
    float step = -1;
    float def = -1;
    rs2_error *e = NULL;

    rs2_get_option_range((const rs2_options *) handle, (rs2_option) option, &min, &max, &step, &def, &e);
    handle_error(env, e);

    jclass clazz = (*env)->GetObjectClass(env, outParams);

    jfieldID minField = (*env)->GetFieldID(env, clazz, "min", "F");
    jfieldID maxField = (*env)->GetFieldID(env, clazz, "max", "F");
    jfieldID stepField = (*env)->GetFieldID(env, clazz, "step", "F");
    jfieldID defField = (*env)->GetFieldID(env, clazz, "def", "F");

    (*env)->SetFloatField(env, outParams, minField, min);
    (*env)->SetFloatField(env, outParams, maxField, max);
    (*env)->SetFloatField(env, outParams, stepField, step);
    (*env)->SetFloatField(env, outParams, defField, def);
}

JNIEXPORT jboolean JNICALL
Java_com_intel_realsense_librealsense_Options_nIsReadOnly(JNIEnv *env, jclass type, jlong handle,
                                                          jint option) {
    rs2_error* e = NULL;
    int rv = rs2_is_option_read_only((const rs2_options *) handle, (rs2_option) option, &e);
    handle_error(env, e);
    return rv > 0;
}

JNIEXPORT jstring JNICALL
Java_com_intel_realsense_librealsense_Options_nGetDescription(JNIEnv *env, jclass type,
                                                              jlong handle, jint option) {
    rs2_error* e = NULL;
    const char *rv = rs2_get_option_description((const rs2_options *) handle, (rs2_option) option, &e);
    handle_error(env, e);
    return (*env)->NewStringUTF(env, rv);
}

JNIEXPORT jstring JNICALL
Java_com_intel_realsense_librealsense_Options_nGetValueDescription(JNIEnv *env, jclass type,
                                                                   jlong handle, jint option,
                                                                   jfloat value) {
    rs2_error* e = NULL;
    const char *rv = rs2_get_option_value_description((const rs2_options *) handle, (rs2_option) option, value, &e);
    handle_error(env, e);
    return (*env)->NewStringUTF(env, rv);
}
