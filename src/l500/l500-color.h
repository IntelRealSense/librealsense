// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2018 Intel Corporation. All Rights Reserved.

#pragma once

#include <vector>
#include <string>
#include <map>

#include "l500-device.h"
#include "stream.h"
#include "l500-depth.h"
#include "algo/thermal-loop/l500-thermal-loop.h"

namespace librealsense
{

    const rs2_distortion l500_distortion = RS2_DISTORTION_BROWN_CONRADY;

    class l500_color
        : public virtual l500_device
    {
    public:
        std::shared_ptr<synthetic_sensor> create_color_device(std::shared_ptr<context> ctx,
            const std::vector<platform::uvc_device_info>& color_devices_info);

        l500_color(std::shared_ptr<context> ctx,
            const platform::backend_device_group& group);

        l500_color_sensor * get_color_sensor() override;

        std::vector<tagged_profile> get_profiles_tags() const override;

    protected:
        std::shared_ptr<stream_interface> _color_stream;

    private:
        friend class l500_color_sensor;

        uint8_t _color_device_idx = -1;

        lazy<ivcam2::intrinsic_rgb> _color_intrinsics_table;
        lazy<std::vector<uint8_t>> _color_extrinsics_table_raw;
        std::shared_ptr<lazy<rs2_extrinsics>> _color_extrinsic;
        lazy< algo::thermal_loop::l500::thermal_calibration_table > _thermal_table;

        ivcam2::intrinsic_rgb read_intrinsics_table() const;
        std::vector<uint8_t> get_raw_extrinsics_table() const;
    };
    class l500_color_sensor
        : public synthetic_sensor
        , public video_sensor_interface
        , public color_sensor
    {
    public:
        explicit l500_color_sensor(l500_color* owner,
            std::shared_ptr<uvc_sensor> uvc_sensor,
            std::shared_ptr<context> ctx,
            std::map<uint32_t, rs2_format> l500_color_fourcc_to_rs2_format,
            std::map<uint32_t, rs2_stream> l500_color_fourcc_to_rs2_stream)
            : synthetic_sensor("RGB Camera", uvc_sensor, owner, l500_color_fourcc_to_rs2_format, l500_color_fourcc_to_rs2_stream),
            _owner(owner),
            _state(sensor_state::CLOSED)
        {
        }
        rs2_intrinsics get_raw_intrinsics( uint32_t width, uint32_t height ) const;

        rs2_intrinsics get_intrinsics( const stream_profile& profile ) const override;



        void set_k_thermal_intrinsics( rs2_intrinsics const & intr );
        void reset_k_thermal_intrinsics(); 
        algo::thermal_loop::l500::thermal_calibration_table get_thermal_table() const;

        stream_profiles init_stream_profiles() override
        {
            auto lock = environment::get_instance().get_extrinsics_graph().lock();

            auto&& results = synthetic_sensor::init_stream_profiles();

            for (auto&& p : results)
            {
                // Register stream types
                if (p->get_stream_type() == RS2_STREAM_COLOR)
                {
                    assign_stream(_owner->_color_stream, p);
                }

                // Register intrinsics
                auto&& video = dynamic_cast<video_stream_profile_interface*>(p.get());
                const auto&& profile = to_profile(p.get());
                std::weak_ptr<l500_color_sensor> wp =
                    std::dynamic_pointer_cast<l500_color_sensor>(this->shared_from_this());
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

        processing_blocks get_recommended_processing_blocks() const override
        {
            return get_color_recommended_proccesing_blocks();
        }


        // Opens the color sensor profile, if the sensor is opened by calibration process,
        // It will close it and reopen with the requested profile.
        void open(const stream_profiles& requests) override;

        // Close the color sensor
        void close() override;
    
        // Start the color sensor streaming
        void start(frame_callback_ptr callback) override;
        
        // Stops the color sensor streaming
        void stop() override;
        
    private:
        l500_color* const _owner;
        action_delayer _action_delayer;
        std::mutex _state_mutex;

        // Intrinsics from the the last successful AC( if there was one ) with k - thermal correction,
        // We save it normalized such that it can be applied to each resolution
        std::shared_ptr< rs2_intrinsics > _k_thermal_intrinsics;

        enum class sensor_state
        {
            CLOSED,
            OWNED_BY_USER
        };

        std::atomic< sensor_state > _state;

        void delayed_start(frame_callback_ptr callback)
        {
            LOG_DEBUG("Starting color sensor...");
            // The delay is here as a work around to a firmware bug [RS5-5453]
            _action_delayer.do_after_delay([&]() { synthetic_sensor::start(callback); });
            LOG_DEBUG("Color sensor started");
        }

        void delayed_stop()
        {
            LOG_DEBUG("Stopping color sensor...");
            // The delay is here as a work around to a firmware bug [RS5-5453]
            _action_delayer.do_after_delay([&]() { synthetic_sensor::stop(); });
            LOG_DEBUG("Color sensor stopped");
        }

        std::string state_to_string(sensor_state state);

        void set_sensor_state(sensor_state state)
        {
            LOG_DEBUG( "Sensor state changed from: " << state_to_string( _state )
                                          << " to: " << state_to_string( state ) );
            _state = state;
        }

    };

}
