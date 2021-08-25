/* License: Apache 2.0. See LICENSE file in root directory. */
/* Copyright(c) 2021 Intel Corporation. All Rights Reserved. */

#include "jni_common.h"
#include <jni.h>
#include "error.h"
#include "../../../include/librealsense2/rs.h"
#include "../../../include/librealsense2/hpp/rs_frame.hpp"


// helper method for converting rs2_stream_profile_list into jlongArray
jlongArray rs_jni_convert_stream_profiles(JNIEnv *env, std::shared_ptr<rs2_stream_profile_list> list)
{
    rs2_error* e = NULL;
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
