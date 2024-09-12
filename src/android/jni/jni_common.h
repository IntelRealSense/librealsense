/* License: Apache 2.0. See LICENSE file in root directory. */
/* Copyright(c) 2021 Intel Corporation. All Rights Reserved. */

#ifndef LIBREALSENSE_JNI_COMMON_H
#define LIBREALSENSE_JNI_COMMON_H

#include <jni.h>
#include "../../../include/librealsense2/rs.h"
#include "../../../include/librealsense2/hpp/rs_frame.hpp"

jlongArray rs_jni_convert_stream_profiles(JNIEnv *env, std::shared_ptr<rs2_stream_profile_list> list);

#endif
