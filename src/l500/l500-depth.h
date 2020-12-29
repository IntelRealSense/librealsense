// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2018 Intel Corporation. All Rights Reserved.

#pragma once

#include <vector>
#include <mutex>
#include <string>
#include <map>

#include "l500-device.h"
#include "context.h"
#include "backend.h"
#include "hw-monitor.h"
#include "image.h"
#include "stream.h"
#include "l500-private.h"
#include "error-handling.h"
#include "l500-options.h"
#include "calibrated-sensor.h"
#include "max-usable-range-sensor.h"
#include "debug-stream-sensor.h"

namespace librealsense
{
    class l500_depth : public virtual l500_device
    {
    public:

        ivcam2::intrinsic_depth read_intrinsics_table() const;

        l500_depth(std::shared_ptr<context> ctx,
            const platform::backend_device_group& group);

        ~l500_depth() { stop_temperatures_reader(); }

        std::vector<tagged_profile> get_profiles_tags() const override;

        std::shared_ptr<matcher> create_matcher(const frame_holder& frame) const override;
    };

    class l500_depth_sensor_interface: public recordable<l500_depth_sensor_interface>
    {
    public:
        virtual ivcam2::intrinsic_depth get_intrinsic() const = 0;
        virtual float read_baseline() const = 0;
        virtual ~l500_depth_sensor_interface() = default;
    };
    MAP_EXTENSION(RS2_EXTENSION_L500_DEPTH_SENSOR, librealsense::l500_depth_sensor_interface);

    class l500_depth_sensor_snapshot : public l500_depth_sensor_interface, public extension_snapshot
    {
    public:
        l500_depth_sensor_snapshot(ivcam2::intrinsic_depth intrinsic, float baseline):
            _intrinsic(intrinsic), _baseline(baseline)
        {}

        ivcam2::intrinsic_depth get_intrinsic() const override
        {
            return _intrinsic;
        }
        float read_baseline() const override
        {
            return _baseline;

        }
        void update(std::shared_ptr<extension_snapshot> ext) override
        {
            if (auto api = As<l500_depth_sensor_interface>(ext))
            {
                _intrinsic = api->get_intrinsic();
            }  
        }

        void create_snapshot(std::shared_ptr<l500_depth_sensor_interface>& snapshot) const override
        {
            snapshot = std::make_shared<l500_depth_sensor_snapshot>(get_intrinsic(), read_baseline());
        }

        void enable_recording(std::function<void(const l500_depth_sensor_interface&)> recording_function) override
        {}

        ~l500_depth_sensor_snapshot() {}

    protected:
        ivcam2::intrinsic_depth _intrinsic;
        float _baseline;
    };

    class l500_depth_sensor
        : public synthetic_sensor
        , public video_sensor_interface
        , public virtual depth_sensor
        , public virtual l500_depth_sensor_interface
        , public calibrated_sensor
        , public max_usable_range_sensor
        , public debug_stream_sensor
    {
    public:
        explicit l500_depth_sensor(
            l500_device * owner,
            std::shared_ptr< uvc_sensor > uvc_sensor,
            std::map< uint32_t, rs2_format > l500_depth_sourcc_to_rs2_format_map,
            std::map< uint32_t, rs2_stream > l500_depth_sourcc_to_rs2_stream_map
        );
        
        ~l500_depth_sensor();

        std::vector<rs2_option> get_supported_options() const override
        {
            std::vector<rs2_option> options;
            for (auto opt : _options)
            {
                if (std::find_if(_owner->_advanced_options.begin(), _owner->_advanced_options.end(), [opt](rs2_option o) { return o == opt.first;}) != _owner->_advanced_options.end())
                    continue;

                 options.push_back(opt.first);
            }

            for (auto option : _owner->_advanced_options)
                options.push_back(option);

            return options;
        }

        static ivcam2::intrinsic_params get_intrinsic_params(const uint32_t width, const uint32_t height, ivcam2::intrinsic_depth intrinsic)
        {
            auto num_of_res = intrinsic.resolution.num_of_resolutions;

            for (auto i = 0; i < num_of_res; i++)
            {
                auto model_world = intrinsic.resolution.intrinsic_resolution[i].world;
                auto model_raw = intrinsic.resolution.intrinsic_resolution[i].raw;

                if (model_world.pinhole_cam_model.height == height && model_world.pinhole_cam_model.width == width)
                    return model_world;
                else if (model_raw.pinhole_cam_model.height == height && model_raw.pinhole_cam_model.width == width)
                    return  model_raw;
            }
            throw std::runtime_error(to_string() << "intrinsics for resolution " << width << "," << height << " doesn't exist");
        }

        rs2_intrinsics get_intrinsics(const stream_profile& profile) const override
        {
            using namespace ivcam2;

            auto intrinsic_params = get_intrinsic_params(profile.width, profile.height, get_intrinsic());

            rs2_intrinsics intrinsics = { 0 };
            intrinsics.width = intrinsic_params.pinhole_cam_model.width;
            intrinsics.height = intrinsic_params.pinhole_cam_model.height;
            intrinsics.fx = intrinsic_params.pinhole_cam_model.ipm.focal_length.x;
            intrinsics.fy = intrinsic_params.pinhole_cam_model.ipm.focal_length.y;
            intrinsics.ppx = intrinsic_params.pinhole_cam_model.ipm.principal_point.x;
            intrinsics.ppy = intrinsic_params.pinhole_cam_model.ipm.principal_point.y;

            intrinsics.coeffs[0] = intrinsic_params.pinhole_cam_model.distort.radial_k1;
            intrinsics.coeffs[1] = intrinsic_params.pinhole_cam_model.distort.radial_k2;
            intrinsics.coeffs[2] = intrinsic_params.pinhole_cam_model.distort.tangential_p1;
            intrinsics.coeffs[3] = intrinsic_params.pinhole_cam_model.distort.tangential_p2;
            intrinsics.coeffs[4] = intrinsic_params.pinhole_cam_model.distort.radial_k3;

            intrinsics.model = RS2_DISTORTION_NONE;
            return intrinsics;
        }

        // calibrated_sensor
        void override_intrinsics( rs2_intrinsics const & intr ) override;
        void override_extrinsics( rs2_extrinsics const & extr ) override;
        rs2_dsm_params get_dsm_params() const override;
        void override_dsm_params( rs2_dsm_params const & dsm_params ) override;
        void reset_calibration() override;

        stream_profiles init_stream_profiles() override
        {
            auto lock = environment::get_instance().get_extrinsics_graph().lock();

            auto&& results = synthetic_sensor::init_stream_profiles();
            for (auto&& p : results)
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
                auto&& video = dynamic_cast<video_stream_profile_interface*>(p.get());

                const auto&& profile = to_profile(p.get());
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

        ivcam2::intrinsic_depth get_intrinsic() const override
        {
            using namespace ivcam2;
            return *_owner->_calib_table;
        }

        float get_max_usable_depth_range() const override;

        stream_profiles get_debug_stream_profiles() const override;

        void create_snapshot(std::shared_ptr<depth_sensor>& snapshot) const override
        {
            snapshot = std::make_shared<depth_sensor_snapshot>(get_depth_scale());
        }
        void enable_recording(std::function<void(const depth_sensor&)> recording_function) override
        {
            get_option(RS2_OPTION_DEPTH_UNITS).enable_recording([this, recording_function](const option& o) {
                recording_function(*this);
            });
        }

        void create_snapshot(std::shared_ptr<l500_depth_sensor_interface>& snapshot) const  override
        {
            snapshot = std::make_shared<l500_depth_sensor_snapshot>(get_intrinsic(), read_baseline());
        }

        void enable_recording(std::function<void(const l500_depth_sensor_interface&)> recording_function) override
        {}

        static processing_blocks get_l500_recommended_proccesing_blocks();

        processing_blocks get_recommended_processing_blocks() const override
        {
            return get_l500_recommended_proccesing_blocks();
        };

        std::shared_ptr< stream_profile_interface > is_color_sensor_needed() const;

        int read_algo_version();
        float read_baseline() const override;
        float read_znorm();

        void start(frame_callback_ptr callback) override;
        void open(const stream_profiles& requests) override;
        void stop() override;
        float get_depth_offset() const;
        bool is_max_range_preset() const;

    private:
        action_delayer _action_delayer;
        l500_device * const _owner;
        float _depth_units;
        stream_profiles _user_requests;
        stream_profiles _validator_requests;
    };
}
