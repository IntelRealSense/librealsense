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

    class zo_point_option_x : public option_base
    {
    public:
        zo_point_option_x(int min, int max, int step, int def, l500_device* owner, lazy < std::pair<int, int>>& zo_point, const std::string& desc)
            : option_base({ static_cast<float>(min),
                           static_cast<float>(max),
                           static_cast<float>(step),
                           static_cast<float>(def) }), _owner(owner), _zo_point(zo_point)
        {}
        float query() const override;
        void set(float value) override
        {
            _zo_point->first = static_cast<int>(value);
        }
        bool is_enabled() const override { return true; }
        const char* get_description() const override { return _desc.c_str(); }

    private:
        l500_device* _owner;
        lazy < std::pair<int, int>>& _zo_point;
        std::string _desc;
    };

    class zo_point_option_y : public option_base
    {
    public:
        zo_point_option_y(int min, int max, int step, int def, l500_device* owner, lazy < std::pair<int, int>>& zo_point, const std::string& desc)
            : option_base({ static_cast<float>(min),
                           static_cast<float>(max),
                           static_cast<float>(step),
                           static_cast<float>(def)}), _owner(owner), _zo_point(zo_point)
        {}
        float query() const override;
        void set(float value) override
        {
            _zo_point->second = static_cast<int>(value);
        }
        bool is_enabled() const override { return true; }
        const char* get_description() const override { return _desc.c_str(); }

    private:
        l500_device* _owner;
        lazy < std::pair<int, int>>& _zo_point;
        std::string _desc;
    };

    class l500_depth_sensor : public uvc_sensor, public video_sensor_interface, public depth_sensor
    {
    public:
        explicit l500_depth_sensor(l500_device* owner, std::shared_ptr<platform::uvc_device> uvc_device,
            std::unique_ptr<frame_timestamp_reader> timestamp_reader)
            : uvc_sensor("L500 Depth Sensor", uvc_device, move(timestamp_reader), owner), _owner(owner)
        {
            _zo_point = [&]() { return read_zo_point(); };

            _zo_point_x_option = std::make_shared<zo_point_option_x>(
                20,
                640,
                1,
                0,
                owner,
                _zo_point,
                "zero order point x");

            register_option(static_cast<rs2_option>(RS2_OPTION_ZERO_ORDER_POINT_X), _zo_point_x_option);

            _zo_point_y_option = std::make_shared<zo_point_option_y>(
                20,
                640,
                1,
                0,
                owner,
                _zo_point,
                "zero order point x");

            register_option(static_cast<rs2_option>(RS2_OPTION_ZERO_ORDER_POINT_Y), _zo_point_y_option);

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

            for (auto i = 0; i < num_of_res; i++)
            {
                auto model_world = intrinsic->resolution.intrinsic_resolution[i].world.pinhole_cam_model;
                auto model_raw = intrinsic->resolution.intrinsic_resolution[i].raw.pinhole_cam_model;

                if (model_world.height == profile.height && model_world.width == profile.width)
                    cam_model = model_world;
                else if (model_raw.height == profile.height && model_raw.width == profile.width)
                    cam_model = model_raw;
                else
                    continue;

                rs2_intrinsics intrinsics;
                intrinsics.width = cam_model.width;
                intrinsics.height = cam_model.height;
                intrinsics.fx = cam_model.ipm.focal_length.x;
                intrinsics.fy = cam_model.ipm.focal_length.y;
                intrinsics.ppx = cam_model.ipm.principal_point.x;
                intrinsics.ppy = cam_model.ipm.principal_point.y;
                return intrinsics;
            }
            throw std::runtime_error(to_string() << "intrinsics for resolution " << profile.width << "," << profile.height << " doesn't exist");
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

        static processing_blocks get_l500_recommended_proccesing_blocks(std::shared_ptr<option> zo_point_x, std::shared_ptr<option> zo_point_y);

        processing_blocks get_recommended_processing_blocks() const override
        {
            return get_l500_recommended_proccesing_blocks(_zo_point_x_option, _zo_point_y_option);
        };

        std::shared_ptr<option> get_zo_point_x() const
        {
            return _zo_point_x_option;
        }
        std::shared_ptr<option> get_zo_point_y() const
        {
            return _zo_point_y_option;
        }
        std::pair<int, int> read_zo_point();
        int read_algo_version();
        float read_baseline();
        float read_znorm();

    private:
        const l500_device* _owner;
        float _depth_units;
        std::shared_ptr<option> _zo_point_x_option;
        std::shared_ptr<option> _zo_point_y_option;
        lazy<std::pair<int, int>> _zo_point;
    };
}
