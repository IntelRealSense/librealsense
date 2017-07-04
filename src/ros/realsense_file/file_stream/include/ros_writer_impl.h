// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once
#include <string>
#include <rosbag/rosbag_storage/include/rosbag/bag.h>
#include "include/ros_writer.h"
#include "include/status.h"
#include "include/file_types.h"

namespace rs
{
	namespace file_format
	{
		class ros_writer_impl : public ros_writer
		{
		public:
			virtual ~ros_writer_impl()
			{
				m_file->close();
			}

			ros_writer_impl(std::string file_path);

			template<class T>
			status write(std::string const& topic, file_types::nanoseconds const& time, T const& msg);

		private:
			std::shared_ptr<rosbag::Bag>    m_file;
		};

		template<class T>
		status ros_writer_impl::write(std::string const& topic, file_types::nanoseconds const& time, T const& msg)
		{
			try
			{
				if (time == file_types::nanoseconds::min())
				{
					m_file->write(topic, ros::TIME_MIN, msg);
				}
				else
				{
					std::chrono::duration<uint32_t> sec = std::chrono::duration_cast<std::chrono::duration<uint32_t>>(time);
					file_types::nanoseconds range = time - std::chrono::duration_cast<file_types::nanoseconds>(sec);
					ros::Time capture_time = ros::Time(sec.count(),
						std::chrono::duration_cast<std::chrono::duration<uint32_t, std::nano>>(range).count());
					m_file->write(topic, capture_time, msg);
				}
			}
			catch (rosbag::BagIOException&)
			{
				return status_file_write_failed;
			}
			return status_no_error;
		}
	}
}
