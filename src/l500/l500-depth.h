// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2018 Intel Corporation. All Rights Reserved.

#pragma once

#include <vector>
#include <mutex>
#include <string>
#include "l500-device.h"
#include "context.h"
#include "backend.h"
#include "hw-monitor.h"
#include "image.h"
#include "stream.h"
#include "l500-private.h"
#include "error-handling.h"

namespace librealsense
{

    class l500_depth : public virtual l500_device
    {
    public:
        std::vector<uint8_t> get_raw_calibration_table() const;

        l500_depth(std::shared_ptr<context> ctx,
            const platform::backend_device_group& group);

        std::vector<tagged_profile> get_profiles_tags() const override;

        std::shared_ptr<matcher> create_matcher(const frame_holder& frame) const override;

    };

    class l500_depth_sensor : public uvc_sensor, public video_sensor_interface, public depth_sensor
    {
    public:
        explicit l500_depth_sensor(l500_device* owner, std::shared_ptr<platform::uvc_device> uvc_device,
            std::unique_ptr<frame_timestamp_reader> timestamp_reader)
            : uvc_sensor("L500 Depth Sensor", uvc_device, move(timestamp_reader), owner), _owner(owner)
        {
            register_option(RS2_OPTION_DEPTH_UNITS, std::make_shared<const_value_option>("Number of meters represented by a single depth unit",
                lazy<float>([&]() {
                return read_znorm(); })));
        }

        rs2_intrinsics get_intrinsics(const stream_profile& profile) const override
        {
            using namespace ivcam2;
            auto intrinsic = check_calib<intrinsic_depth>(*_owner->_calib_table_raw);

            auto num_of_res = intrinsic->resolution.num_of_resolutions;
            pinhole_camera_model cam_model;

            auto intrinsic_params = get_intrinsic_params(profile.width, profile.height);

            rs2_intrinsics intrinsics;
            intrinsics.width = intrinsic_params.pinhole_cam_model.width;
            intrinsics.height = intrinsic_params.pinhole_cam_model.height;
            intrinsics.fx = intrinsic_params.pinhole_cam_model.ipm.focal_length.x;
            intrinsics.fy = intrinsic_params.pinhole_cam_model.ipm.focal_length.y;
            intrinsics.ppx = intrinsic_params.pinhole_cam_model.ipm.principal_point.x;
            intrinsics.ppy = intrinsic_params.pinhole_cam_model.ipm.principal_point.y;
            return intrinsics;
        }

        stream_profiles init_stream_profiles() override
        {
            auto lock = environment::get_instance().get_extrinsics_graph().lock();

            auto results = uvc_sensor::init_stream_profiles();
            for (auto p : results)
            {
                // Register stream types
                if (p->get_stream_type() == RS2_STREAM_DEPTH)
                {
                    assign_stream(_owner->_depth_stream, p);
                }
                else if (p->get_stream_type() == RS2_STREAM_INFRARED)
                {
                    assign_stream(_owner->_ir_stream, p);
                }
                else if (p->get_stream_type() == RS2_STREAM_CONFIDENCE)
                {
                    assign_stream(_owner->_confidence_stream, p);
                }

                // Register intrinsics
                auto video = dynamic_cast<video_stream_profile_interface*>(p.get());

                auto profile = to_profile(p.get());
                std::weak_ptr<l500_depth_sensor> wp =
                    std::dynamic_pointer_cast<l500_depth_sensor>(this->shared_from_this());

                video->set_intrinsics([profile, wp]()
                {
                    auto sp = wp.lock();
                    if (sp)
                        return sp->get_intrinsics(profile);
                    else
                        return rs2_intrinsics{};
                });
            }

            return results;
        }

        ivcam2::intrinsic_params get_intrinsic_params(const uint32_t width, const uint32_t height) const
        {
            using namespace ivcam2;
            auto intrinsic = check_calib<intrinsic_depth>(*_owner->_calib_table_raw);

            auto num_of_res = intrinsic->resolution.num_of_resolutions;
            intrinsic_params cam_model;

            for (auto i = 0; i < num_of_res; i++)
            {
                auto model_world = intrinsic->resolution.intrinsic_resolution[i].world;
                auto model_raw = intrinsic->resolution.intrinsic_resolution[i].raw;

                if (model_world.pinhole_cam_model.height == height && model_world.pinhole_cam_model.width == width)
                    return model_world;
                else if (model_raw.pinhole_cam_model.height == height && model_raw.pinhole_cam_model.width == width)
                    return  model_raw;
            }
            throw std::runtime_error(to_string() << "intrinsics for resolution " << width << "," << height << " doesn't exist");
        }

        float get_depth_scale() const override { return get_option(RS2_OPTION_DEPTH_UNITS).query(); }

        void create_snapshot(std::shared_ptr<depth_sensor>& snapshot) const  override
        {
            snapshot = std::make_shared<depth_sensor_snapshot>(get_depth_scale());
        }
        void enable_recording(std::function<void(const depth_sensor&)> recording_function) override
        {
            get_option(RS2_OPTION_DEPTH_UNITS).enable_recording([this, recording_function](const option& o) {
                recording_function(*this);
            });
        }

        static processing_blocks get_l500_recommended_proccesing_blocks();

        processing_blocks get_recommended_processing_blocks() const override
        {
            return get_l500_recommended_proccesing_blocks();
        };

        int read_algo_version();
        float read_baseline();
        float read_znorm();

    private:
        const l500_device* _owner;
        float _depth_units;
    };
}
