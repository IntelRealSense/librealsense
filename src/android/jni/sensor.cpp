// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#include <jni.h>
#include <memory>
#include <vector>
#include "error.h"

#include "../../../include/librealsense2/rs.h"

extern "C"
JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_Sensor_nRelease(JNIEnv *env, jclass type, jlong handle) {
    rs2_delete_sensor(reinterpret_cast<rs2_sensor *>(handle));
}

extern "C"
JNIEXPORT jlongArray JNICALL
Java_com_intel_realsense_librealsense_Sensor_nGetStreamProfiles(JNIEnv *env, jclass type,
                                                                jlong handle) {
    rs2_error* e = nullptr;
    std::shared_ptr<rs2_stream_profile_list> list(
            rs2_get_stream_profiles(reinterpret_cast<rs2_sensor *>(handle), &e),
            rs2_delete_stream_profiles_list);
    handle_error(env, e);

    auto size = rs2_get_stream_profiles_count(list.get(), &e);
    handle_error(env, e);

    std::vector<const rs2_stream_profile*> profiles;

    for (auto i = 0; i < size; i++)
    {
        auto sp = rs2_get_stream_profile(list.get(), i, &e);
        handle_error(env, e);
        profiles.push_back(sp);
    }

    jlongArray rv = env->NewLongArray(profiles.size());
    env->SetLongArrayRegion(rv, 0, profiles.size(), reinterpret_cast<const jlong *>(profiles.data()));
    return rv;
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_intel_realsense_librealsense_Sensor_nIsSensorExtendableTo(JNIEnv *env, jclass type,
                                                                   jlong handle, jint extension) {
    rs2_error *e = NULL;
    int rv = rs2_is_sensor_extendable_to(reinterpret_cast<const rs2_sensor *>(handle),
                                         static_cast<rs2_extension>(extension), &e);
    handle_error(env, e);
    return rv > 0;
}


extern "C"
JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_RoiSensor_nSetRegionOfInterest(JNIEnv *env, jclass clazz,
                                                                     jlong handle, jint min_x,
                                                                     jint min_y, jint max_x,
                                                                     jint max_y) {
    rs2_error* e = nullptr;
    rs2_set_region_of_interest(reinterpret_cast<rs2_sensor *>(handle), min_x, min_y, max_x, max_y, &e);
    handle_error(env, e);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_RoiSensor_nGetRegionOfInterest(JNIEnv *env, jclass type,
                                                                     jlong handle, jobject roi) {
    int min_x, min_y, max_x, max_y;
    rs2_error *e = nullptr;
    rs2_get_region_of_interest(reinterpret_cast<rs2_sensor *>(handle), &min_x, &min_y, &max_x, &max_y, &e);
    handle_error(env, e);

    if(e)
        return;

    jclass clazz = env->GetObjectClass(roi);

    jfieldID min_x_field = env->GetFieldID(clazz, "minX", "I");
    jfieldID min_y_field = env->GetFieldID(clazz, "minY", "I");
    jfieldID max_x_field = env->GetFieldID(clazz, "maxX", "I");
    jfieldID max_y_field = env->GetFieldID(clazz, "maxY", "I");

    env->SetIntField(roi, min_x_field, min_x);
    env->SetIntField(roi, min_y_field, min_y);
    env->SetIntField(roi, max_x_field, max_x);
    env->SetIntField(roi, max_y_field, max_y);
}

extern "C"
JNIEXPORT jfloat JNICALL
Java_com_intel_realsense_librealsense_DepthSensor_nGetDepthScale(JNIEnv *env, jclass clazz,
                                                                     jlong handle) {
    rs2_error* e = nullptr;
    float depthScale = rs2_get_depth_scale(reinterpret_cast<rs2_sensor *>(handle), &e);
    handle_error(env, e);
    return depthScale;
}