// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once

#include <sensor_msgs/Imu.h>
#include "stream_data.h"
namespace librealsense
{
    namespace file_format
    {
        namespace ros_data_objects
        {
            struct motion_info
            {
                file_types::nanoseconds capture_time;
                uint32_t device_id;
                file_types::motion_type type;
                float data[3];
                file_types::seconds timestamp;
                uint32_t frame_number;
            };

            class motion_sample : public sample
            {
            public:
                motion_sample(const motion_info& info): m_info(info)
                {
                    std::memcpy(m_info.data, info.data, sizeof(m_info.data));
                }

                virtual file_types::sample_type get_type() const override
                {
                    return file_types::st_motion;
                }

                virtual motion_info get_info() const
                {
                    return m_info;
                }

                virtual void set_info(const motion_info& info)
                {
                    m_info = info;
                    std::memcpy(m_info.data, info.data, sizeof(m_info.data));
                }

                virtual void write_data(data_object_writer& file) override
                {
                    std::string topic = get_topic(m_info.type, m_info.device_id);
                    std::string type_str;
                    if(conversions::convert(m_info.type, type_str) == false)
                    {
                        //return status_param_unsupported;
                    }
                    if(type_str == file_types::motion_stream_type::ACCL)
                    {
                        sensor_msgs::ImuPtr accelerometer = create_accelerometer_message(topic, m_info.frame_number);
                        accelerometer->linear_acceleration.x = m_info.data[0];
                        accelerometer->linear_acceleration.y = m_info.data[1];
                        accelerometer->linear_acceleration.z = m_info.data[2];

                        accelerometer->header.stamp = ros::Time(m_info.timestamp.count());
                        file.write(topic, m_info.capture_time, accelerometer);
                    }
                    else if(type_str == file_types::motion_stream_type::GYRO)
                    {
                        sensor_msgs::ImuPtr gyrometer = create_gyrometer_message(topic, m_info.frame_number);
                        gyrometer->angular_velocity.x = m_info.data[0];
                        gyrometer->angular_velocity.y = m_info.data[1];
                        gyrometer->angular_velocity.z = m_info.data[2];

                        gyrometer->header.stamp = ros::Time(m_info.timestamp.count());
                        gyrometer->header.seq = m_info.frame_number;
                        file.write(topic, m_info.capture_time, gyrometer);
                    }else
                    {
                        throw librealsense::invalid_value_exception(type_str + "is not supported");
                    }
                }

                static std::string get_topic(file_types::motion_type type, uint32_t device_id)
                {
                    std::string type_str;
                    conversions::convert(type, type_str);
                    return "/imu/" + type_str + "/imu_raw/" + std::to_string(device_id);
                }

            private:
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
                motion_info m_info;
            };
        }
    }

}
