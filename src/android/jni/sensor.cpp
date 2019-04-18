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
