// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include "include/msgs/sensor_msgs/Imu.h"
#include "rosbag/view.h"
#include "file_stream/include/conversions.h"
#include "file_stream/include/topic.h"
#include "file_stream/include/ros_writer_impl.h"
#include "file_stream/include/data_objects/motion_sample_impl.h"

using namespace rs::file_format;
using namespace rs::file_format::ros_data_objects;
using namespace rs::file_format::file_types;

inline sensor_msgs::ImuPtr create_imu_message(const std::string& frame_id, const uint64_t seq_id)
{
    sensor_msgs::ImuPtr imuMessage(new sensor_msgs::Imu());
    imuMessage->header.frame_id = frame_id;
    imuMessage->header.seq = static_cast<uint32_t>(seq_id);
    return imuMessage;
}

inline sensor_msgs::ImuPtr create_accelerometer_message(const std::string& frame_id, const uint64_t seq_id)
{
    sensor_msgs::ImuPtr accelerometerMessagePtr = create_imu_message(frame_id, seq_id);
    accelerometerMessagePtr->orientation_covariance[0] = -1;
    accelerometerMessagePtr->angular_velocity_covariance[0] = -1;

    return accelerometerMessagePtr;
}

inline sensor_msgs::ImuPtr create_gyrometer_message(const std::string& frame_id, const uint64_t seq_id)
{
    sensor_msgs::ImuPtr gyrometerMessagePtr = create_imu_message(frame_id, seq_id);
    gyrometerMessagePtr->orientation_covariance[0] = -1;
    gyrometerMessagePtr->linear_acceleration_covariance[0] = -1;

    return gyrometerMessagePtr;
}

std::string motion_sample::get_topic(file_types::motion_type type, uint32_t device_id = 0)
{
    std::string type_str;
    conversions::convert(type, type_str);
    return "/imu/" + type_str + "/imu_raw/" + std::to_string(device_id);
}

std::shared_ptr<motion_sample> motion_sample::create(const motion_info& info)
{
    return std::make_shared<motion_sample_impl>(info);
}

motion_sample_impl::motion_sample_impl(const motion_info &info): m_info(info)
{
    std::memcpy(m_info.data, info.data, sizeof(m_info.data));
}

file_types::sample_type motion_sample_impl::get_type() const { return file_types::st_motion; }

status motion_sample_impl::write_data(std::shared_ptr<ros_writer> file)
{
    std::string topic = get_topic(m_info.type, m_info.device_id);
    std::string type_str;
    if(conversions::convert(m_info.type, type_str) == false)
    {
        return status_param_unsupported;
    }
    auto file_instance = std::static_pointer_cast<ros_writer_impl>(file);
    if(type_str == file_types::motion_stream_type::ACCL)
    {
        sensor_msgs::ImuPtr accelerometer = create_accelerometer_message(topic, m_info.frame_number);
        accelerometer->linear_acceleration.x = m_info.data[0];
        accelerometer->linear_acceleration.y = m_info.data[1];
        accelerometer->linear_acceleration.z = m_info.data[2];

        accelerometer->header.stamp = ros::Time(m_info.timestamp.count());
        if(file_instance->write(topic, m_info.capture_time, accelerometer) != status_no_error)
        {
            return status_file_write_failed;
        }
    }
    else if(type_str == file_types::motion_stream_type::GYRO)
    {
        sensor_msgs::ImuPtr gyrometer = create_gyrometer_message(topic, m_info.frame_number);
        gyrometer->angular_velocity.x = m_info.data[0];
        gyrometer->angular_velocity.y = m_info.data[1];
        gyrometer->angular_velocity.z = m_info.data[2];

        gyrometer->header.stamp = ros::Time(m_info.timestamp.count());
        gyrometer->header.seq = m_info.frame_number;
        if(file_instance->write(topic, m_info.capture_time, gyrometer) != status_no_error)
        {
            return status_file_write_failed;
        }
    }else
    {
        return status_feature_unsupported;
    }
    return status_no_error;
}

motion_info motion_sample_impl::get_info() const
{
    return m_info;
}

void motion_sample_impl::set_info(const motion_info &info)
{
    m_info = info;
    std::memcpy(m_info.data, info.data, sizeof(m_info.data));
}

