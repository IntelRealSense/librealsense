// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once
#include "file_types.h"
#include "std_msgs/Float32.h"
#include "std_msgs/String.h"
#include "realsense_msgs/motion_intrinsics.h"
#include "realsense_msgs/extrinsics.h"
#include "rosbag/bag.h"

namespace rs //TOOD: rename to librealsense
{
    namespace file_format
    {
        namespace conversions
        {
            bool convert(file_types::pixel_format source, std::string& target);

            bool convert(const std::string& source, file_types::pixel_format& target);

            bool convert(file_types::compression_type source, std::string& target);

            bool convert(file_types::motion_type source, std::string& target);

            bool convert(const std::string& source, file_types::motion_type& target);

            void convert(const realsense_msgs::motion_intrinsics& source, rs::file_format::file_types::motion_intrinsics& target);

            void convert(const rs::file_format::file_types::motion_intrinsics& source, realsense_msgs::motion_intrinsics& target);

            void convert(const realsense_msgs::extrinsics& source, rs::file_format::file_types::extrinsics& target);

            void convert(const rs::file_format::file_types::extrinsics& source, realsense_msgs::extrinsics& target);
        }
    }
}
