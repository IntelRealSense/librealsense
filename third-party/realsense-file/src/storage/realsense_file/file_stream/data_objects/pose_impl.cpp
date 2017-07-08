// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include "realsense_msgs/pose.h"
#include "file_stream/include/topic.h"
#include "file_stream/include/data_objects/pose_impl.h"
#include "file_stream/include/conversions.h"
#include "file_stream/include/ros_writer_impl.h"

using namespace rs::file_format;
using namespace rs::file_format::ros_data_objects;
using namespace rs::file_format::file_types;

std::string pose::get_topic(uint32_t device_id)
{
    return "/camera/rs_6DoF/" + std::to_string(device_id);
}

std::shared_ptr<pose> pose::create(const pose_info& info)
{
    return std::make_shared<pose_impl>(info);
}

pose_impl::pose_impl(const pose_info &info) : m_info(info)
{
}

file_types::sample_type pose_impl::get_type() const { return file_types::st_pose; }

status pose_impl::write_data(std::shared_ptr<ros_writer> file)
{
    realsense_msgs::pose msg;

	msg.translation.x = m_info.translation.x;
	msg.translation.y = m_info.translation.y;
	msg.translation.z = m_info.translation.z;

	msg.rotation.x = m_info.rotation.x;
	msg.rotation.y = m_info.rotation.y;
	msg.rotation.z = m_info.rotation.z;
	msg.rotation.w = m_info.rotation.w;

	msg.velocity.x = m_info.velocity.x;
	msg.velocity.y = m_info.velocity.y;
	msg.velocity.z = m_info.velocity.z;

	msg.angular_velocity.x = m_info.angular_velocity.x;
	msg.angular_velocity.y = m_info.angular_velocity.y;
	msg.angular_velocity.z = m_info.angular_velocity.z;

	msg.acceleration.x = m_info.acceleration.x;
	msg.acceleration.y = m_info.acceleration.y;
	msg.acceleration.z = m_info.acceleration.z;

	msg.angular_acceleration.x = m_info.angular_acceleration.x;
	msg.angular_acceleration.y = m_info.angular_acceleration.y;
	msg.angular_acceleration.z = m_info.angular_acceleration.z;

	msg.timestamp = m_info.timestamp.count();
	msg.system_timestamp = m_info.system_timestamp.count();

    auto file_instance = std::static_pointer_cast<ros_writer_impl>(file);
    return file_instance->write(get_topic(m_info.device_id), m_info.capture_time, msg);

}

pose_info pose_impl::get_info() const
{
    return m_info;
}

void pose_impl::set_info(const pose_info &info)
{
	m_info = info;
}
