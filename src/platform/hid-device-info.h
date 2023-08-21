// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include <sstream>


namespace librealsense {
namespace platform {


struct hid_device_info
{
    std::string id;
    std::string vid;
    std::string pid;
    std::string unique_id;
    std::string device_path;
    std::string serial_number;

    operator std::string()
    {
        std::ostringstream s;
        s << "id- " << id << "\nvid- " << std::hex << vid << "\npid- " << std::hex << pid << "\nunique_id- "
          << unique_id << "\npath- " << device_path;

        return s.str();
    }
};


inline bool operator==( const hid_device_info & a, const hid_device_info & b )
{
    return ( a.id == b.id ) && ( a.vid == b.vid ) && ( a.pid == b.pid ) && ( a.unique_id == b.unique_id )
        && ( a.device_path == b.device_path );
}


}  // namespace platform
}  // namespace librealsense
