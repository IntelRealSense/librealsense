// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once
#include <string>
#include <memory>
#include "ros_writer.h"
#include "ros/status.h"
#include "ros/file_types.h"
#include "ros/exception.h"
#include "rosbag/bag.h"
#include "types.h"
namespace rs
{
	namespace file_format
	{
		class ros_writer
		{
		public:
			ros_writer(std::string file_path)
			{
				try
				{
					m_bag.open(file_path, rosbag::BagMode::Write);
					m_bag.setCompression(rosbag::CompressionType::LZ4);
				}
                catch(rosbag::BagIOException& e)
				{
					throw librealsense::invalid_value_exception(std::string("Failed to open file: \"") + file_path + std::string("\" for writing. Exception: ") + e.what());
				}
			}

			template<class T>
			void write(std::string const& topic, rs::file_format::file_types::nanoseconds const& time, T const& msg)
			{
				try
				{
					if (time == rs::file_format::file_types::nanoseconds::min())
					{
						m_bag.write(topic, ros::TIME_MIN, msg);
					}
					else
					{
						std::chrono::duration<uint32_t> sec = std::chrono::duration_cast<std::chrono::duration<uint32_t>>(time);
						rs::file_format::file_types::nanoseconds nanos = time - std::chrono::duration_cast<rs::file_format::file_types::nanoseconds>(sec);
						auto unanos = std::chrono::duration_cast<std::chrono::duration<uint32_t, std::nano>>(nanos);
						ros::Time capture_time = ros::Time(sec.count(),unanos.count());
						m_bag.write(topic, capture_time, msg);
					}
				}
				catch (rosbag::BagIOException& e)
				{
					throw librealsense::io_exception(std::string("Ros Writer: Failed to write topic: \"") + topic
														 + std::string("\" to file. (rosbag error: ") + e.what());
				}
			}

		private:
			rosbag::Bag m_bag;
		};


	}
}
