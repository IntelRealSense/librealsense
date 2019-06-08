// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#include <jni.h>
#include <memory>
#include <vector>
#include "error.h"

#include "../../../include/librealsense2/rs.h"



extern "C" JNIEXPORT jboolean JNICALL
Java_com_intel_realsense_librealsense_Device_nSupportsInfo(JNIEnv *env, jclass type, jlong handle,
                                                           jint info) {
    rs2_error *e = NULL;
    auto rv = rs2_supports_device_info(reinterpret_cast<const rs2_device *>(handle),
                                       static_cast<rs2_camera_info>(info), &e);
    handle_error(env, e);
    return rv > 0;
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_intel_realsense_librealsense_Device_nGetInfo(JNIEnv *env, jclass type, jlong handle,
                                                      jint info) {
    rs2_error *e = NULL;
    const char* rv = rs2_get_device_info(reinterpret_cast<const rs2_device *>(handle),
                                         static_cast<rs2_camera_info>(info), &e);
    handle_error(env, e);
    if (NULL == rv)
        rv = "";
    return env->NewStringUTF(rv);
}

extern "C" JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_Device_nRelease(JNIEnv *env, jclass type, jlong handle) {
    rs2_delete_device(reinterpret_cast<rs2_device *>(handle));
}

extern "C"
JNIEXPORT jlongArray JNICALL
Java_com_intel_realsense_librealsense_Device_nQuerySensors(JNIEnv *env, jclass type, jlong handle) {
    rs2_error* e = nullptr;
    std::shared_ptr<rs2_sensor_list> list(
            rs2_query_sensors(reinterpret_cast<const rs2_device *>(handle), &e),
            rs2_delete_sensor_list);
    handle_error(env, e);

    auto size = rs2_get_sensors_count(list.get(), &e);
    handle_error(env, e);

    std::vector<rs2_sensor*> sensors;
    for (auto i = 0; i < size; i++)
    {
        auto s = rs2_create_sensor(list.get(), i, &e);
        handle_error(env, e);
        sensors.push_back(s);
    }
    jlongArray rv = env->NewLongArray(sensors.size());
    env->SetLongArrayRegion(rv, 0, sensors.size(), reinterpret_cast<const jlong *>(sensors.data()));
    return rv;

}
