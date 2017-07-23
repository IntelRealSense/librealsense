// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once
#include <memory>
#include <rosbag/bag.h>

#include "media/ros/status.h"
#include "media/ros/file_types.h"


namespace librealsense
{
    class data_object_writer
    {
        rosbag::Bag& m_bag;
    public:
        data_object_writer(rosbag::Bag& bag) : m_bag(bag) {}

        template <typename T>
        void write(std::string const& topic,file_format::file_types::nanoseconds const& time, T const& msg)
        {
            try
            {
                if (time ==file_format::file_types::nanoseconds::min())
                {
                    m_bag.write(topic, ros::TIME_MIN, msg);
                }
                else
                {
                    std::chrono::duration<uint32_t> sec = std::chrono::duration_cast<std::chrono::duration<uint32_t>>(time);
                    file_format::file_types::nanoseconds nanos = time - std::chrono::duration_cast<file_format::file_types::nanoseconds>(sec);
                    auto unanos = std::chrono::duration_cast<std::chrono::duration<uint32_t, std::nano>>(nanos);
                    ros::Time capture_time = ros::Time(sec.count(),unanos.count());
                    m_bag.write(topic, capture_time, msg);
                }
            }
            catch (rosbag::BagIOException& e)
            {
                throw librealsense::io_exception(to_string() << "Ros Writer: Failed to write topic: \"" << topic << "\" to file. (rosbag error: " << e.what() << ")");
            }
        }
    };
    namespace file_format
    {
        namespace ros_data_objects
        {

            class stream_data
            {
            public:
                virtual ~stream_data(){}
                virtual void write_data(data_object_writer& writer) = 0;
            };

            struct stream_info : stream_data
            {
                virtual ~stream_info(){}
            };

            struct sample : stream_data
            {
                virtual ~sample(){}
                virtual file_types::sample_type get_type() const = 0;
            };
        }
    }
}
