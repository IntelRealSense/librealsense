// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2018 Intel Corporation. All Rights Reserved.

#pragma once

#include <vector>
#include <mutex>
#include <string>
#include "device.h"
#include "context.h"
#include "backend.h"
#include "hw-monitor.h"
#include "image.h"
#include "stream.h"
#include "l500-private.h"
#include "error-handling.h"

namespace librealsense
{

    class l500_depth : public virtual device, public debug_interface
    {
    public:
        std::shared_ptr<uvc_sensor> create_depth_device(std::shared_ptr<context> ctx,
            const std::vector<platform::uvc_device_info>& all_device_infos);

        std::vector<uint8_t> send_receive_raw_data(const std::vector<uint8_t>& input) override
        {
            return _hw_monitor->send(input);
        }

        void hardware_reset() override
        {
            force_hardware_reset();
        }

        uvc_sensor& get_depth_sensor() { return dynamic_cast<uvc_sensor&>(get_sensor(_depth_device_idx)); }

        std::vector<uint8_t> get_raw_calibration_table() const;

        l500_depth(std::shared_ptr<context> ctx,
            const platform::backend_device_group& group);

        void create_snapshot(std::shared_ptr<debug_interface>& snapshot) const override;
        void enable_recording(std::function<void(const debug_interface&)> record_action) override;
        std::vector<tagged_profile> get_profiles_tags() const override;

        std::shared_ptr<matcher> create_matcher(const frame_holder& frame) const override;

    protected:
        friend class l500_depth_sensor;

        const uint8_t _depth_device_idx;
        std::shared_ptr<hw_monitor> _hw_monitor;
        std::shared_ptr<stream_interface> _depth_stream;
        std::shared_ptr<stream_interface> _ir_stream;
        std::shared_ptr<stream_interface> _confidence_stream;

        std::unique_ptr<polling_error_handler> _polling_error_handler;

        lazy<std::vector<uint8_t>> _calib_table_raw;
        void force_hardware_reset() const;
    };

    class zo_point_option_x : public option_base
    {
    public:
        zo_point_option_x(int min, int max, int step, int def, l500_depth* owner, lazy < std::pair<int, int>>& zo_point, const std::string& desc)
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
        l500_depth* _owner;
        lazy < std::pair<int, int>>& _zo_point;
        std::string _desc;
    };

    class zo_point_option_y : public option_base
    {
    public:
        zo_point_option_y(int min, int max, int step, int def, l500_depth* owner, lazy < std::pair<int, int>>& zo_point, const std::string& desc)
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
        l500_depth* _owner;
        lazy < std::pair<int, int>>& _zo_point;
        std::string _desc;
    };

    class l500_depth_sensor : public uvc_sensor, public video_sensor_interface, public depth_sensor
    {
    public:
        explicit l500_depth_sensor(l500_depth* owner, std::shared_ptr<platform::uvc_device> uvc_device,
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
            auto res = *_owner->_calib_table_raw;
            auto intr = (float*)res.data();

            if (res.size() < sizeof(float) * 4)
                throw invalid_value_exception("size of calibration invalid");

            rs2_intrinsics intrinsics;
            intrinsics.width = profile.width;
            intrinsics.height = profile.height;
            intrinsics.fx = std::fabs(intr[0]);
            intrinsics.fy = std::fabs(intr[1]);
            intrinsics.ppx = std::fabs(intr[2]);
            intrinsics.ppy = std::fabs(intr[3]);
            intrinsics.model = RS2_DISTORTION_NONE;
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
        const l500_depth* _owner;
        float _depth_units;
        std::shared_ptr<option> _zo_point_x_option;
        std::shared_ptr<option> _zo_point_y_option;
        lazy<std::pair<int, int>> _zo_point;
    };

    class l500_notification_decoder : public notification_decoder
    {
    public:
        notification decode(int value) override;
    };
}
