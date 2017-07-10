// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once

#include <realsense_msgs/motion_stream_info.h>
#include "stream_data.h"

namespace rs
{
    namespace file_format
    {
        namespace ros_data_objects
        {
            struct motion_stream_data
            {
                file_types::motion_type type;
                uint32_t fps;
                rs2_motion_device_intrinsic intrinsics;
                file_types::stream_extrinsics stream_extrinsics;
                uint32_t device_id;
            };

            class motion_stream_info : public stream_info
            {
			public:
                motion_stream_info(const motion_stream_data &info) : m_info(info)
                {
                    assign(info, m_info);
                }

                virtual motion_stream_data get_info() const
                {
                    return m_info;
                }
                virtual void set_info(const motion_stream_data& info)
                {
                    assign(info, m_info);
                }

                void write_data(rs::file_format::ros_writer& file) override
                {
                    realsense_msgs::motion_stream_info msg;
                    msg.fps = m_info.fps;

                    if(conversions::convert(m_info.type, msg.motion_type) == false)
                    {
                        //return status_param_unsupported;
                    }

                    conversions::convert(m_info.intrinsics, msg.stream_intrinsics);
                    conversions::convert(m_info.stream_extrinsics.extrinsics_data, msg.stream_extrinsics.extrinsics);
                    msg.stream_extrinsics.reference_point_id = m_info.stream_extrinsics.reference_point_id;

                    file.write(get_topic(m_info.device_id), file_types::nanoseconds::min(), msg);
                }

                static std::string get_topic(uint32_t device_id)
                {
                    return "/camera/rs_motion_stream_info/" + std::to_string(device_id);
                }

            private:
                motion_stream_data m_info;

                void assign(const motion_stream_data& in_info, motion_stream_data& out_info)
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
            };
        }
    }
}
