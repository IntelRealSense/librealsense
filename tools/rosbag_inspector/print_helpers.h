// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once

#include <string>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <array>

#include "../../third-party/realsense-file/rosbag/rosbag_storage/include/rosbag/bag.h"
#include "../../third-party/realsense-file/rosbag/msgs/sensor_msgs/Imu.h"
#include "../../third-party/realsense-file/rosbag/msgs/sensor_msgs/Image.h"
#include "../../third-party/realsense-file/rosbag/msgs/diagnostic_msgs/KeyValue.h"
#include "../../third-party/realsense-file/rosbag/msgs/std_msgs/UInt32.h"
#include "../../third-party/realsense-file/rosbag/msgs/std_msgs/String.h"
#include "../../third-party/realsense-file/rosbag/msgs/std_msgs/Float32.h"
#include "../../third-party/realsense-file/rosbag/msgs/realsense_msgs/StreamInfo.h"
#include "../../third-party/realsense-file/rosbag/msgs/realsense_msgs/ImuIntrinsic.h"
#include "../../third-party/realsense-file/rosbag/msgs/sensor_msgs/CameraInfo.h"
#include "../../third-party/realsense-file/rosbag/msgs/sensor_msgs/TimeReference.h"
#include "../../third-party/realsense-file/rosbag/msgs/geometry_msgs/Transform.h"
#include "../../third-party/realsense-file/rosbag/msgs/geometry_msgs/Twist.h"
#include "../../third-party/realsense-file/rosbag/msgs/geometry_msgs/Accel.h"
#include "../../third-party/realsense-file/rosbag/msgs/realsense_legacy_msgs/legacy_headers.h"

namespace rosbag_inspector
{
    struct tmpstringstream
    {
        std::ostringstream ss;
        template<class T> tmpstringstream & operator << (const T & val) { ss << val; return *this; }
        operator std::string() const { return ss.str(); }
    };

    inline std::string pretty_time(std::chrono::nanoseconds d)
    {
        auto hhh = std::chrono::duration_cast<std::chrono::hours>(d);
        d -= hhh;
        auto mm = std::chrono::duration_cast<std::chrono::minutes>(d);
        d -= mm;
        auto ss = std::chrono::duration_cast<std::chrono::seconds>(d);
        d -= ss;
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(d);

        std::ostringstream stream;
        stream << std::setfill('0') << std::setw(3) << hhh.count() << ':' <<
            std::setfill('0') << std::setw(2) << mm.count() << ':' <<
            std::setfill('0') << std::setw(2) << ss.count() << '.' <<
            std::setfill('0') << std::setw(3) << ms.count();
        return stream.str();
    }


    // Streamers


    template <typename Container>
    std::ostream& print_container(std::ostream& os, const Container& c)
    {
        for (size_t i = 0; i < c.size(); ++i)
        {
            if (i != 0)
                os << ",";
            os << c[i];
        }
        return os;
    }

    template <size_t N, typename T>
    std::ostream& operator<<(std::ostream& os, const std::array<T, N>& arr)
    {
        return print_container(os, arr);
    }

    template <typename T>
    std::ostream& operator<<(std::ostream& os, const std::vector<T>& v)
    {
        return print_container(os, v);
    }

    std::ostream& operator<<(std::ostream& os, rosbag::compression::CompressionType c)
    {
        switch (c)
        {
        case rosbag::CompressionType::Uncompressed: os << "Uncompressed"; break;
        case rosbag::CompressionType::BZ2: os << "BZ2"; break;
        case rosbag::CompressionType::LZ4: os << "LZ4"; break;
        default: break;
        }
        return os;
    }

    template <typename ROS_TYPE>
    inline typename ROS_TYPE::ConstPtr try_instantiate(const rosbag::MessageInstance& m)
    {
        if(m.isType<ROS_TYPE>())
            return m.instantiate<ROS_TYPE>();
        return nullptr;
    }

    std::ostream& operator<<(std::ostream& os, const rosbag::MessageInstance& m)
    {
        if (auto data = try_instantiate<std_msgs::UInt32>(m))
        {
            os << "Value : " << data->data << std::endl;
        }
        else if (auto data = try_instantiate<std_msgs::String>(m))
        {
            os << "Value : " << data->data << std::endl;
        }
        else if (auto data = try_instantiate<std_msgs::Float32>(m))
        {
            os << "Value : " << data->data << std::endl;
        }
        else if (auto data = try_instantiate<diagnostic_msgs::KeyValue>(m))
        {
            auto kvp = data;
            os << "Key   : " << kvp->key << std::endl;
            os << "Value : " << kvp->value << std::endl;
        }
        else if (auto data = try_instantiate<realsense_msgs::StreamInfo>(m))
        {
            auto stream_info = data;
            os << "Encoding       : " << stream_info->encoding << std::endl;
            os << "FPS            : " << stream_info->fps << std::endl;
            os << "Is Recommended : " << (stream_info->is_recommended ? "True" : "False") << std::endl;
        }
        else if (auto data = try_instantiate<sensor_msgs::CameraInfo>(m))
        {
            auto camera_info = data;
            os << "Width      : " << camera_info->width << std::endl;
            os << "Height     : " << camera_info->height << std::endl;
            os << "Intrinsics : " << std::endl;
            os << "  Focal Point      : " << camera_info->K[0] << ", " << camera_info->K[4] << std::endl;
            os << "  Principal Point  : " << camera_info->K[2] << ", " << camera_info->K[5] << std::endl;
            os << "  Coefficients     : "
                << camera_info->D[0] << ", "
                << camera_info->D[1] << ", "
                << camera_info->D[2] << ", "
                << camera_info->D[3] << ", "
                << camera_info->D[4] << std::endl;
            os << "  Distortion Model : " << camera_info->distortion_model << std::endl;
        }
        else if (auto data = try_instantiate<realsense_msgs::ImuIntrinsic>(m))
        {
            os << "bias_variances : " << data->bias_variances << '\n';
            os << "data : " << data->data << '\n';
            os << "noise_variances : " << data->noise_variances << '\n';
        }
        else if (auto data = try_instantiate<sensor_msgs::Image>(m))
        {
            auto image = data;
            os << "Header       : \n";
            os << "  frame_id           : " << image->header.frame_id << std::endl;
            os << "  Frame Number (seq) : " << image->header.seq << std::endl;
            os << "  stamp              : " << image->header.stamp << std::endl;
            os << "Encoding     : " << image->encoding << std::endl;
            os << "Width        : " << image->width << std::endl;
            os << "Height       : " << image->height << std::endl;
            os << "Step         : " << image->step << std::endl;
            //os << "Frame Number : " << image->header.seq << std::endl;
            //os << "Timestamp    : " << pretty_time(std::chrono::nanoseconds(image->header.stamp.toNSec())) << std::endl;
        }
        else if (auto data = try_instantiate<sensor_msgs::Imu>(m))
        {
            auto imu = data;
            os << "header : " << imu->header << '\n';
            os << "orientation : " << imu->orientation << '\n';
            os << "orientation_covariance : " << imu->orientation_covariance << '\n';
            os << "angular_velocity : " << imu->angular_velocity << '\n';
            os << "angular_velocity_covariance : " << imu->angular_velocity_covariance << '\n';
            os << "linear_acceleration : " << imu->linear_acceleration << '\n';
            os << "linear_acceleration_covariance : " << imu->linear_acceleration_covariance << '\n';
            os << "orientation_covariance : " << imu->orientation_covariance << '\n';
            os << "angular_velocity_covariance : " << imu->angular_velocity_covariance << '\n';
            os << "linear_acceleration_covariance : " << imu->linear_acceleration_covariance << '\n';
        }
        else if (auto data = try_instantiate<sensor_msgs::TimeReference>(m))
        {
            auto tr = data;
            os << "  Header        : " << tr->header << std::endl;
            os << "  Source        : " << tr->source << std::endl;
            os << "  TimeReference : " << tr->time_ref << std::endl;
        }
        else if (auto data = try_instantiate<geometry_msgs::Transform>(m))
        {
            auto tf = data;
            os << "    Rotation    : " << tf->rotation << std::endl;
            os << "    Translation : " << tf->translation << std::endl;
        }
        else if (try_instantiate<geometry_msgs::Twist>(m) || try_instantiate<geometry_msgs::Accel>(m))
        {
            if (m.getDataType().find("Twist") != std::string::npos)
            {
                auto twist = try_instantiate<geometry_msgs::Twist>(m);
                os << "Angular Velocity: " << twist->angular << std::endl;
                os << "Linear  Velocity: " << twist->linear << std::endl;
            }
            if (m.getDataType().find("Accel") != std::string::npos)
            {
                auto accel = try_instantiate<geometry_msgs::Accel>(m);
                os << "Angular Acceleration: " << accel->angular << std::endl;
                os << "Linear  Acceleration: " << accel->linear << std::endl;
            }
        }

        /*************************************************************/
        /*************************************************************/
        /*                     Legacy Messages                       */
        /*************************************************************/
        /*************************************************************/

        else if (auto data = try_instantiate<realsense_legacy_msgs::compressed_frame_info>(m))
        {
            os << "encoding: " << data->encoding << '\n';
            os << "frame_metadata: " << data->frame_metadata << '\n';
            os << "height: " << data->height << '\n';
            os << "step: " << data->step << '\n';
            os << "system_time: " << data->system_time << '\n';
            os << "time_stamp_domain: " << data->time_stamp_domain << '\n';
            os << "width: " << data->width << '\n';
        }
        else if (auto data = try_instantiate<realsense_legacy_msgs::controller_command>(m))
        {
            os << "controller_id : " << data->controller_id << '\n';
            os << "mac_address : " << data->mac_address << '\n';
            os << "type : " << data->type << '\n';
        }
        else if (auto data = try_instantiate<realsense_legacy_msgs::controller_event>(m))
        {
            os << "controller_id : " << data->controller_id << '\n';
            os << "mac_address : " << data->mac_address << '\n';
            os << "type : " << data->type << '\n';
            os << "timestamp : " << data->timestamp << '\n';
        }
        else if (auto data = try_instantiate<realsense_legacy_msgs::controller_vendor_data>(m))
        {
            os << "controller_id : " << data->controller_id << '\n';
            os << "data : " << data->data << '\n';
            os << "timestamp : " << data->timestamp << '\n';
            os << "vendor_data_source_index : " << data->vendor_data_source_index << '\n';
            os << "vendor_data_source_type : " << data->vendor_data_source_type << '\n';
        }
        else if (auto data = try_instantiate<realsense_legacy_msgs::extrinsics>(m))
        {
            os << "  Extrinsics : " << std::endl;
            os << "    Rotation    : " << data->rotation << std::endl;
            os << "    Translation : " << data->translation << std::endl;
        }
        else if (auto data = try_instantiate<realsense_legacy_msgs::frame_info>(m))
        {
            os << "frame_metadata :" << data->frame_metadata << '\n';
            os << "system_time :" << data->system_time << '\n';
            os << "time_stamp_domain :" << data->time_stamp_domain << '\n';
        }
        else if (auto data = try_instantiate<realsense_legacy_msgs::metadata>(m))
        {
            os << "type : " << data->type << '\n';
            os << "data : " << data->data << '\n';
        }
        else if (auto data = try_instantiate<realsense_legacy_msgs::motion_intrinsics>(m))
        {
            os << "bias_variances : " << data->bias_variances << '\n';
            os << "data : " << data->data << '\n';
            os << "noise_variances : " << data->noise_variances << '\n';
        }
        else if (auto data = try_instantiate<realsense_legacy_msgs::motion_stream_info>(m))
        {
            os << "fps : " << data->fps << '\n';
            os << "motion_type : " << data->motion_type << '\n';
            os << "stream_extrinsics : " << data->stream_extrinsics << '\n';
            os << "stream_intrinsics : " << data->stream_intrinsics << '\n';
        }
        else if (auto data = try_instantiate<realsense_legacy_msgs::occupancy_map>(m))
        {
            os << "accuracy : " << data->accuracy << '\n';
            os << "reserved : " << data->reserved << '\n';
            os << "tiles : " << data->tiles << '\n';
            os << "tile_count : " << data->tile_count << '\n';
        }
        else if (auto data = try_instantiate<realsense_legacy_msgs::pose>(m))
        {
            os << "translation : " << data->translation << '\n';
            os << "rotation : " << data->rotation << '\n';
            os << "velocity : " << data->velocity << '\n';
            os << "angular_velocity : " << data->angular_velocity << '\n';
            os << "acceleration : " << data->acceleration << '\n';
            os << "angular_acceleration : " << data->angular_acceleration << '\n';
            os << "timestamp : " << data->timestamp << '\n';
            os << "system_timestamp : " << data->system_timestamp << '\n';
        }
        else if (auto data = try_instantiate<realsense_legacy_msgs::stream_extrinsics>(m))
        {
            os << "extrinsics : " << data->extrinsics << '\n';
            os << "reference_point_id : " << data->reference_point_id << '\n';
        }
        else if (auto data = try_instantiate<realsense_legacy_msgs::stream_info>(m))
        {
            os << "stream_type : " << data->stream_type << '\n';
            os << "fps : " << data->fps << '\n';
            os << "camera_info : " << data->camera_info << '\n';
            os << "stream_extrinsics : " << data->stream_extrinsics << '\n';
            os << "width : " << data->width << '\n';
            os << "height : " << data->height << '\n';
            os << "encoding : " << data->encoding << '\n';
        }
        else if (auto data = try_instantiate<realsense_legacy_msgs::vendor_data>(m))
        {
            os << "name : " << data->name << '\n';
            os << "value : " << data->value << '\n';
        }
        else
        {
            os << m.getDataType() << '\n';
        }

        return os;
    }


}