// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include "include/msgs/realsense_msgs/stream_info.h"
#include "file_stream/include/conversions.h"
#include "file_stream/include/topic.h"
#include "file_stream/include/ros_writer_impl.h"
#include "file_stream/include/data_objects/image_stream_info_impl.h"

using namespace rs::file_format;
using namespace rs::file_format::ros_data_objects;
using namespace rs::file_format::file_types;

std::string image_stream_info::get_topic(const uint32_t& device_id)
{
    return "/camera/rs_stream_info/" + std::to_string(device_id);
}

std::shared_ptr<image_stream_info> image_stream_info::create(const image_stream_data& info)
{
    return std::make_shared<image_stream_info_impl>(info);
}

void image_stream_info_impl::assign(const image_stream_data& in_info, image_stream_data& out_info)
{
    out_info = in_info;
    out_info.intrinsics.fx = in_info.intrinsics.fx;
    out_info.intrinsics.ppx = in_info.intrinsics.ppx;
    out_info.intrinsics.fy = in_info.intrinsics.fy;
    out_info.intrinsics.ppy = in_info.intrinsics.ppy;
    std::memcpy(out_info.intrinsics.coeffs,
                in_info.intrinsics.coeffs,
                sizeof(out_info.intrinsics.coeffs));

    std::memcpy(out_info.stream_extrinsics.extrinsics_data.rotation,
                in_info.stream_extrinsics.extrinsics_data.rotation,
                sizeof(out_info.stream_extrinsics.extrinsics_data.rotation));
    std::memcpy(out_info.stream_extrinsics.extrinsics_data.translation,
                in_info.stream_extrinsics.extrinsics_data.translation,
                sizeof(out_info.stream_extrinsics.extrinsics_data.translation));
    out_info.stream_extrinsics.reference_point_id = in_info.stream_extrinsics.reference_point_id;
}


image_stream_info_impl::image_stream_info_impl(const image_stream_data& info) : m_info(info)
{
    assign(info, m_info);
}

status image_stream_info_impl::write_data(std::shared_ptr<ros_writer> file)
{
    realsense_msgs::stream_info msg;
    msg.stream_type = m_info.type;
    msg.fps = m_info.fps;
    msg.width = m_info.width;
    msg.height = m_info.height;
    if(conversions::convert(m_info.format, msg.encoding) == false)
    {
        return status_param_unsupported;
    }
    msg.camera_info.height = m_info.height;
    msg.camera_info.width  = m_info.width;
    msg.camera_info.K[0]   = m_info.intrinsics.fx;
    msg.camera_info.K[2]   = m_info.intrinsics.ppx;

    msg.camera_info.K[4] = m_info.intrinsics.fy;
    msg.camera_info.K[5] = m_info.intrinsics.ppy;
    msg.camera_info.K[8] = 1;
    msg.camera_info.D.assign(m_info.intrinsics.coeffs, m_info.intrinsics.coeffs + 5);
    msg.camera_info.distortion_model = m_info.intrinsics.model;

    conversions::convert(m_info.stream_extrinsics.extrinsics_data, msg.stream_extrinsics.extrinsics);
    msg.stream_extrinsics.reference_point_id = m_info.stream_extrinsics.reference_point_id;
    auto file_instance = std::static_pointer_cast<ros_writer_impl>(file);
    return file_instance->write(get_topic(m_info.device_id), file_types::nanoseconds::min(), msg);
}

const image_stream_data &image_stream_info_impl::get_info() const
{
    return m_info;
}

void image_stream_info_impl::set_info(const image_stream_data &info)
{
    assign(info, m_info);
}
