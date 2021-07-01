// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#include <jni.h>
#include <memory>
#include <vector>
#include "error.h"

#include "../../../include/librealsense2/rs.h"

#include "jni_logging.h"
#include "frame_callback.h"

extern "C"
JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_Sensor_nOpen(JNIEnv *env, jclass type, jlong handle, jlong sp) {
    rs2_error* e = nullptr;
    rs2_open(reinterpret_cast<rs2_sensor *>(handle), reinterpret_cast<rs2_stream_profile *>(sp), &e);
    handle_error(env, e);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_Sensor_nOpenMultiple(JNIEnv *env, jclass type, jlong device_handle,
        jlongArray profiles_handle, int num_of_profiles) {
    // retrieving profiles from array
    jboolean isCopy = 0;
    jlong* profiles = env->GetLongArrayElements(profiles_handle, &isCopy);
    // building C++ profiles vector
    rs2_error* e = nullptr;
    std::vector<const rs2_stream_profile*> profs;
    profs.reserve(num_of_profiles);
    for (int i = 0; i < num_of_profiles; ++i)
    {
        auto p = reinterpret_cast<const rs2_stream_profile*>(profiles[i]);
        profs.push_back(p);
    }
    // API call
    rs2_open_multiple(reinterpret_cast<rs2_sensor *>(device_handle),
                      profs.data(), static_cast<int>(num_of_profiles), &e);
    handle_error(env, e);
}

static frame_callback_data sdata = {NULL, 0, JNI_FALSE, NULL, NULL};

extern "C"
JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_Sensor_nStart(JNIEnv *env, jclass type, jlong handle, jobject jcb) {
    rs2_error* e = nullptr;

    if (rs_jni_callback_init(env, jcb, &sdata) != true) return;

    auto cb = [&](rs2::frame f) {
        rs_jni_cb(f, &sdata);
    };

    rs2_start_cpp(reinterpret_cast<rs2_sensor *>(handle), new rs2::frame_callback<decltype(cb)>(cb), &e);
    handle_error(env, e);
}


extern "C"
JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_Sensor_nStop(JNIEnv *env, jclass type, jlong handle) {
    rs2_error* e = nullptr;
    rs_jni_cleanup(env, &sdata);
    rs2_stop(reinterpret_cast<rs2_sensor *>(handle), &e);
    handle_error(env, e);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_Sensor_nClose(JNIEnv *env, jclass type, jlong handle) {
    rs2_error* e = nullptr;
    rs2_close(reinterpret_cast<rs2_sensor *>(handle), &e);
    handle_error(env, e);
}

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

    // jlong is 64-bit, but pointer in profiles could be 32-bit, copy element by element
    jlongArray rv = env->NewLongArray(profiles.size());
    for (auto i = 0; i < size; i++)
    {
        env->SetLongArrayRegion(rv, i, 1, reinterpret_cast<const jlong *>(&profiles[i]));
    }
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
