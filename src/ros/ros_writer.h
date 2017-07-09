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
namespace rs
{
	namespace file_format
	{
		class ros_writer
		{
		public:
			virtual ~ros_writer()
			{
				m_file->close();
			}

			ros_writer(std::string file_path)
			{
				try
				{
					m_file = std::make_shared<rosbag::Bag>();
					m_file->open(file_path, rosbag::BagMode::Write);
					m_file->setCompression(rosbag::CompressionType::LZ4);
				} catch(rosbag::BagIOException&)
				{
					throw std::runtime_error(std::string("Failed to open file: \"") + file_path + "\"");
				}
			}

			template<class T>
			rs::file_format::status write(std::string const& topic, rs::file_format::file_types::nanoseconds const& time, T const& msg)
			{
				try
				{
					if (time == rs::file_format::file_types::nanoseconds::min())
					{
						m_file->write(topic, ros::TIME_MIN, msg);
					}
					else
					{
						std::chrono::duration<uint32_t> sec = std::chrono::duration_cast<std::chrono::duration<uint32_t>>(time);
						rs::file_format::file_types::nanoseconds range = time - std::chrono::duration_cast<rs::file_format::file_types::nanoseconds>(sec);
						ros::Time capture_time = ros::Time(sec.count(),
														   std::chrono::duration_cast<std::chrono::duration<uint32_t, std::nano>>(range).count());
						m_file->write(topic, capture_time, msg);
					}
				}
				catch (rosbag::BagIOException&)
				{
					return rs::file_format::status::status_file_write_failed;
				}
				return rs::file_format::status::status_no_error;
			}

		private:
			std::shared_ptr<rosbag::Bag> m_file;
		};


	}
}
