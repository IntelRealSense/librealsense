// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "ds5-private.h"

#include "algo.h"
#include "error-handling.h"

namespace rsimpl2
{
    class emitter_option : public uvc_xu_option<uint8_t>
    {
    public:
        const char* get_value_description(float val) const override;
        explicit emitter_option(uvc_endpoint& ep);
    };

    class asic_and_projector_temperature_options : public readonly_option
    {
    public:
        float query() const override;

        option_range get_range() const override;

        bool is_enabled() const override;

        const char* get_description() const override;

        explicit asic_and_projector_temperature_options(uvc_endpoint& ep, rs2_option opt);

    private:
        uvc_endpoint&                 _ep;
        rs2_option                    _option;
    };

    class motion_module_temperature_option : public readonly_option
    {
    public:
        float query() const override;

        option_range get_range() const override;

        bool is_enabled() const override;

        const char* get_description() const override;

        explicit motion_module_temperature_option(hid_endpoint& ep);

    private:
        const std::string custom_sensor_name = "custom";
        const std::string report_name = "data-field-custom-value_2";
        hid_endpoint& _ep;
    };

    class enable_motion_correction : public option
    {
    public:
        void set(float value) override;

        float query() const override;

        option_range get_range() const override
        {
            return option_range{0, 1, 1, 1};
        }

        bool is_enabled() const override { return true; }

        const char* get_description() const override
        {
            return "Enable/Disable Automatic Motion Data Correction";
        }

        enable_motion_correction(endpoint* mm_ep,
                                 ds::imu_intrinsics accel,
                                 ds::imu_intrinsics gyro);

    private:
        std::atomic<bool> _is_enabled;
        ds::imu_intrinsics _accel;
        ds::imu_intrinsics _gyro;
    };

    class enable_auto_exposure_option : public option
    {
    public:
        void set(float value) override;

        float query() const override;

        option_range get_range() const override
        {
            return option_range{0, 1, 1, 1};
        }

        bool is_enabled() const override { return true; }

        const char* get_description() const override
        {
            return "Enable/disable auto-exposure";
        }

        enable_auto_exposure_option(uvc_endpoint* fisheye_ep,
                                    std::shared_ptr<auto_exposure_mechanism> auto_exposure,
                                    std::shared_ptr<auto_exposure_state> auto_exposure_state);

    private:
        std::shared_ptr<auto_exposure_state>         _auto_exposure_state;
        std::atomic<bool>                            _to_add_frames;
        std::shared_ptr<auto_exposure_mechanism>     _auto_exposure;
    };

    class auto_exposure_mode_option : public option
    {
    public:
        auto_exposure_mode_option(std::shared_ptr<auto_exposure_mechanism> auto_exposure,
                                  std::shared_ptr<auto_exposure_state> auto_exposure_state);

        void set(float value) override;

        float query() const override;

        option_range get_range() const override
        {
            return option_range { 0, 2, 1, 0 };
        }

        bool is_enabled() const override { return true; }

        const char* get_description() const override
        {
            return "Auto-Exposure Mode";
        }

        const char* get_value_description(float val) const override;

    private:
        std::shared_ptr<auto_exposure_state>        _auto_exposure_state;
        std::shared_ptr<auto_exposure_mechanism>    _auto_exposure;
    };

    class auto_exposure_antiflicker_rate_option : public option
    {
    public:
        auto_exposure_antiflicker_rate_option(std::shared_ptr<auto_exposure_mechanism> auto_exposure,
                                              std::shared_ptr<auto_exposure_state> auto_exposure_state);

        void set(float value) override;

        float query() const override;

        option_range get_range() const override
        {
            return option_range { 50, 60, 10, 60 };
        }

        bool is_enabled() const override { return true; }

        const char* get_description() const override
        {
            return "Auto-Exposure anti-flicker";
        }

        const char* get_value_description(float val) const override;

    private:
        std::shared_ptr<auto_exposure_state>         _auto_exposure_state;
        std::shared_ptr<auto_exposure_mechanism>     _auto_exposure;
    };

    class depth_scale_option : public option
    {
    public:
        depth_scale_option(hw_monitor& hwm);

        void set(float value) override;
        float query() const override;
        option_range get_range() const override;
        bool is_enabled() const override { return true; }

        const char* get_description() const override
        {
            return "Number of meters represented by a single depth unit";
        }

    private:
        ds::depth_table_control get_depth_table(ds::advnaced_query_mode mode) const;

        lazy<option_range> _range;
        hw_monitor& _hwm;
    };
}
