// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include "file_stream/include/data_objects/motion_stream_info_impl.h"
#include "realsense_msgs/motion_stream_info.h"
#include "file_stream/include/conversions.h"
#include "file_stream/include/topic.h"
#include "file_stream/include/ros_writer_impl.h"

using namespace rs::file_format;
using namespace rs::file_format::ros_data_objects;
using namespace rs::file_format::file_types;

std::string motion_stream_info::get_topic(uint32_t device_id)
{
    return "/camera/rs_motion_stream_info/" + std::to_string(device_id);
}

std::shared_ptr<motion_stream_info> motion_stream_info::create(const motion_stream_data& info)
{
    return std::make_shared<motion_stream_info_impl>(info);
}

void motion_stream_info_impl::assign(const motion_stream_data& in_info, motion_stream_data& out_info)
{
    out_info = in_info;
    std::memcpy(out_info.intrinsics.bias_variances,
                in_info.intrinsics.bias_variances,
                sizeof(out_info.intrinsics.bias_variances));
    std::memcpy(out_info.intrinsics.noise_variances,
                in_info.intrinsics.noise_variances,
                sizeof(out_info.intrinsics.noise_variances));
    std::memcpy(out_info.intrinsics.data,
                in_info.intrinsics.data,
                sizeof(out_info.intrinsics.data));

    std::memcpy(out_info.stream_extrinsics.extrinsics_data.rotation,
                in_info.stream_extrinsics.extrinsics_data.rotation,
                sizeof(out_info.stream_extrinsics.extrinsics_data.rotation));
    std::memcpy(out_info.stream_extrinsics.extrinsics_data.translation,
                in_info.stream_extrinsics.extrinsics_data.translation,
                sizeof(out_info.stream_extrinsics.extrinsics_data.translation));
    out_info.stream_extrinsics.reference_point_id = in_info.stream_extrinsics.reference_point_id;

}

motion_stream_info_impl::~motion_stream_info_impl()
{
}

motion_stream_info_impl::motion_stream_info_impl(const motion_stream_data &info) : m_info(info)
{
    assign(info, m_info);
}

status motion_stream_info_impl::write_data(std::shared_ptr<ros_writer> file)
{
    realsense_msgs::motion_stream_info msg;
    msg.fps = m_info.fps;

    if(conversions::convert(m_info.type, msg.motion_type) == false)
    {
        return status_param_unsupported;
    }

    conversions::convert(m_info.intrinsics, msg.stream_intrinsics);
    conversions::convert(m_info.stream_extrinsics.extrinsics_data, msg.stream_extrinsics.extrinsics);
    msg.stream_extrinsics.reference_point_id = m_info.stream_extrinsics.reference_point_id;

    auto file_instance = std::static_pointer_cast<ros_writer_impl>(file);
    return file_instance->write(get_topic(m_info.device_id), file_types::nanoseconds::min(), msg);
}

motion_stream_data motion_stream_info_impl::get_info() const
{
    return m_info;
}

void motion_stream_info_impl::set_info(const motion_stream_data &info)
{
    assign(info, m_info);
}

