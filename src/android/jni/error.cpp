// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#include "error.h"

void handle_error(JNIEnv *env, rs2_error* error){
    if (error)
    {
        char* message;
        rs2_exception_type h = rs2_get_librealsense_exception_type(error);
        switch (h) {
            case RS2_EXCEPTION_TYPE_CAMERA_DISCONNECTED: message = "RS2_EXCEPTION_TYPE_CAMERA_DISCONNECTED"; break;
            case RS2_EXCEPTION_TYPE_BACKEND: message = "RS2_EXCEPTION_TYPE_BACKEND"; break;
            case RS2_EXCEPTION_TYPE_INVALID_VALUE: message = "RS2_EXCEPTION_TYPE_INVALID_VALUE"; break;
            case RS2_EXCEPTION_TYPE_WRONG_API_CALL_SEQUENCE: message = "RS2_EXCEPTION_TYPE_WRONG_API_CALL_SEQUENCE"; break;
            case RS2_EXCEPTION_TYPE_NOT_IMPLEMENTED: message = "RS2_EXCEPTION_TYPE_NOT_IMPLEMENTED"; break;
            case RS2_EXCEPTION_TYPE_DEVICE_IN_RECOVERY_MODE: message = "RS2_EXCEPTION_TYPE_DEVICE_IN_RECOVERY_MODE"; break;
            default: message = "UNKNOWN_ERROR"; break;
        }
        env->ThrowNew(env->FindClass("java/lang/RuntimeException"), message);
    }
}
