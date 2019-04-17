// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#include <jni.h>
#include "error.h"
#include "../../../include/librealsense2/rs.h"
#include "../../../include/librealsense2/rs_advanced_mode.h"
#include "../../api.h"

extern "C" JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_Device_nToggleAdvancedMode(JNIEnv *env, jclass type,
                                                                 jlong handle, jboolean enable) {
    rs2_error* e = NULL;
    rs2_toggle_advanced_mode(reinterpret_cast<rs2_device *>(handle), enable, &e);
    handle_error(env, e);
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_intel_realsense_librealsense_Device_nIsInAdvancedMode(JNIEnv *env, jclass type,
                                                               jlong handle) {
    rs2_error* e = NULL;
    int rv = -1;
    rs2_is_enabled(reinterpret_cast<rs2_device *>(handle), &rv, &e);
    handle_error(env, e);
    return rv > 0;
}

extern "C" JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_Device_nLoadPresetFromJson(JNIEnv *env, jclass type,
                                                                 jlong handle, jbyteArray data_) {
    jbyte *data = env->GetByteArrayElements(data_, NULL);
    jsize length = env->GetArrayLength(data_);
    rs2_error* e = NULL;
    rs2_load_json(reinterpret_cast<rs2_device *>(handle), data, length, &e);
    handle_error(env, e);
    env->ReleaseByteArrayElements(data_, data, 0);
}

extern "C" JNIEXPORT jbyteArray JNICALL
Java_com_intel_realsense_librealsense_Device_nSerializePresetToJson(JNIEnv *env, jclass type,
                                                                    jlong handle) {
    rs2_error* e = NULL;
    std::shared_ptr<rs2_raw_data_buffer> raw_data_buffer(
            rs2_serialize_json(reinterpret_cast<rs2_device *>(handle), &e),
            [](rs2_raw_data_buffer* buff){ if(buff) delete buff;});
    handle_error(env, e);
    jbyteArray rv = env->NewByteArray(raw_data_buffer->buffer.size());
    env->SetByteArrayRegion(rv, 0, raw_data_buffer->buffer.size(),
                            reinterpret_cast<const jbyte *>(raw_data_buffer->buffer.data()));
    return rv;
}
