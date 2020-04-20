/* License: Apache 2.0. See LICENSE file in root directory. */
/* Copyright(c) 2019 Intel Corporation. All Rights Reserved. */

#ifndef LIBREALSENSE_JNI_ERROR_H
#define LIBREALSENSE_JNI_ERROR_H

#include <jni.h>
#include "../../../include/librealsense2/rs.h"

void handle_error(JNIEnv *env, rs2_error* error);
#endif
