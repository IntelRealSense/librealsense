// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#include "error.h"
#include "../../../include/librealsense2/hpp/rs_types.hpp"

void handle_error(JNIEnv *env, rs2_error* error) {
    if (error)
    {
        const char* message = rs2_get_error_message(error);
        env->ThrowNew(env->FindClass("java/lang/RuntimeException"), message);
    }
}
void handle_error(JNIEnv *env, rs2::error error) {

    const char *message = error.what();
    env->ThrowNew(env->FindClass("java/lang/RuntimeException"), message);
}