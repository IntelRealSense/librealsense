// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#pragma once

#include <sstream>
#include <memory>


namespace librealsense {
namespace platform {


struct mipi_device_info
{
    std::string id;
    uint16_t vid;
    uint16_t pid;
    std::string unique_id;
    std::string device_path;
    std::string dfu_device_path;
    std::string serial_number;

    operator std::string()
    {
        std::ostringstream s;
        s << "id- " << id << "\nvid- " << std::hex << vid << "\npid- " << std::hex << pid << "\nunique_id- "
          << unique_id << "\npath- " << device_path;

        return s.str();
    }
};


inline bool operator==( const mipi_device_info & a, const mipi_device_info & b )
{
    return ( a.id == b.id ) && ( a.vid == b.vid ) && ( a.pid == b.pid ) && ( a.unique_id == b.unique_id )
        && ( a.device_path == b.device_path );
}

}  // namespace platform
}  // namespace librealsense
