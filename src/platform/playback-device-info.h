// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include <string>


namespace librealsense {
namespace platform {


struct playback_device_info
{
    std::string file_path;

    operator std::string() { return file_path; }
};

inline bool operator==( const playback_device_info & a, const playback_device_info & b )
{
    return ( a.file_path == b.file_path );
}


}  // namespace platform
}  // namespace librealsense
