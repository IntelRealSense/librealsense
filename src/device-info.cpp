// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#include "device-info.h"
#include <ostream>


namespace librealsense {


void device_info::to_stream( std::ostream & os ) const
{
    os << get_address();
}


}  // namespace librealsense
