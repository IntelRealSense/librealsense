// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "d400-private.h"
#include "ds/ds-options.h"

#include "algo.h"
#include "error-handling.h"
#include "../../hdr-config.h"

namespace librealsense
{

    class asic_temperature_option_mipi : public readonly_option
    {
    public:
        float query() const override;

        option_range get_range() const override;

        inline bool is_enabled() const override {return true;}

        inline const char* get_description() const override {return "Current Asic Temperature (degree celsius)";}

        explicit asic_temperature_option_mipi(std::shared_ptr<hw_monitor> hwm, rs2_option opt);

    private:
        rs2_option                  _option;
        std::shared_ptr<hw_monitor>  _hw_monitor;
    };

    class projector_temperature_option_mipi : public readonly_option
    {
    public:
        float query() const override;

        option_range get_range() const override;

        inline bool is_enabled() const override {return true;}

        inline const char* get_description() const override {return "Current Projector Temperature (degree celsius)";}

        explicit projector_temperature_option_mipi(std::shared_ptr<hw_monitor> hwm, rs2_option opt);

    private:
        rs2_option                  _option;
        std::shared_ptr<hw_monitor>  _hw_monitor;
    };
    class limits_option;
    class auto_exposure_limit_option : public option_base
    {
    public:
        auto_exposure_limit_option(hw_monitor& hwm, sensor_base* depth_ep, option_range range, std::shared_ptr<limits_option> exposure_limit_enable);
        virtual ~auto_exposure_limit_option() = default;
        virtual void set(float value) override;
        virtual float query() const override;
        virtual option_range get_range() const override;
        virtual bool is_enabled() const override { return true; }
        virtual bool is_read_only() const { return _sensor && _sensor->is_opened(); }
        virtual const char* get_description() const override
        {
            return "Exposure limit is in microseconds. If the requested exposure limit is greater than frame time, it will be set to frame time at runtime. Setting will not take effect until next streaming session.";
        }
        virtual void enable_recording(std::function<void(const option&)> record_action) override { _record_action = record_action; }

    private:
        std::function<void(const option&)> _record_action = [](const option&) {};
        lazy<option_range> _range;
        hw_monitor& _hwm;
        sensor_base* _sensor;
        std::weak_ptr<limits_option> _exposure_limit_toggle;
    };

    class auto_gain_limit_option : public option_base
    {
    public:
        auto_gain_limit_option(hw_monitor& hwm, sensor_base* depth_ep, option_range range, std::shared_ptr<limits_option> gain_limit_enable);
        virtual ~auto_gain_limit_option() = default;
        virtual void set(float value) override;
        virtual float query() const override;
        virtual option_range get_range() const override;
        virtual bool is_enabled() const override { return true; }
        virtual bool is_read_only() const { return _sensor && _sensor->is_opened(); }
        virtual const char* get_description() const override
        {
            return "Gain limits ranges from 16 to 248. If the requested gain limit is less than 16, it will be set to 16. If the requested gain limit is greater than 248, it will be set to 248. Setting will not take effect until next streaming session.";
        }
        virtual void enable_recording(std::function<void(const option&)> record_action) override { _record_action = record_action; }

    private:
        std::function<void(const option&)> _record_action = [](const option&) {};
        lazy<option_range> _range;
        hw_monitor& _hwm;
        sensor_base* _sensor;
        std::weak_ptr<limits_option> _gain_limit_toggle;
    };

    // Auto-Limits Enable/ Disable
    class limits_option : public option
    {
    public:

        limits_option(rs2_option option, option_range range, const char* description, hw_monitor& hwm) :
            _option(option), _toggle_range(range), _description(description), _hwm(hwm) {};

        virtual void set(float value) override
        {
            auto set_limit = _cached_limit;
            if (value == 0) // 0: gain auto-limit is disabled, 1 : gain auto-limit is enabled (all range 16-248 is valid)
                set_limit = 0;

            command cmd_get(ds::AUTO_CALIB);
            cmd_get.param1 = 5;
            std::vector<uint8_t> ret = _hwm.send(cmd_get);
            if (ret.empty())
                throw invalid_value_exception("auto_exposure_limit_option::query result is empty!");

            command cmd(ds::AUTO_CALIB);
            cmd.param1 = 4;
            cmd.param2 = *(reinterpret_cast<uint32_t*>(ret.data()));
            cmd.param3 = static_cast<int>(set_limit);
            if (_option == RS2_OPTION_AUTO_EXPOSURE_LIMIT_TOGGLE)
            {
                cmd.param2 = static_cast<int>(set_limit);
                cmd.param3 = *(reinterpret_cast<uint32_t*>(ret.data() + 4));
            }
            _hwm.send(cmd);
        };
        virtual float query() const override 
        { 
            auto offset = 0;
            if (_option == RS2_OPTION_AUTO_GAIN_LIMIT_TOGGLE)
                offset = 4;
            command cmd(ds::AUTO_CALIB);
            cmd.param1 = 5;
            auto res = _hwm.send(cmd);
            if (res.empty())
                throw invalid_value_exception("auto_exposure_limit_option::query result is empty!");

            auto limit_val = static_cast<float>(*(reinterpret_cast<uint32_t*>(res.data() + offset)));
            if (limit_val == 0)
                return 0;
            return 1;
        };
        virtual option_range get_range() const override { return _toggle_range; };
        virtual bool is_enabled() const override { return true; }
        virtual const char* get_description() const override { return _description; };
        virtual void enable_recording(std::function<void(const option&)> record_action) override { _record_action = record_action; }
        virtual const char* get_value_description(float val) const override
        {
            if (_description_per_value.find(val) != _description_per_value.end())
                return _description_per_value.at(val).c_str();
            return nullptr;
        };
        void set_cached_limit(float value) { _cached_limit = value; };
        float get_cached_limit() { return _cached_limit; };

    private:
        std::function<void(const option&)> _record_action = [](const option&) {};
        rs2_option _option;
        option_range _toggle_range;
        const std::map<float, std::string> _description_per_value;
        float _cached_limit;         // used to cache contol limit value when toggle is switched to off
        const char* _description;
        hw_monitor& _hwm;
    };

    class d400_thermal_monitor;
    class thermal_compensation : public option
    {
    public:
        thermal_compensation(std::shared_ptr<d400_thermal_monitor> monitor,
            std::shared_ptr<option> toggle);

        void set(float value) override;
        float query() const override;

        option_range get_range() const override { return option_range{ 0, 1, 1, 0 }; }
        bool is_enabled() const override { return true; }

        const char* get_description() const override;
        const char* get_value_description(float value) const override;
        void create_snapshot(std::shared_ptr<option>& snapshot) const override;

        void enable_recording(std::function<void(const option&)> record_action) override
        {
            _recording_function = record_action;
        }

    private:
        std::shared_ptr<d400_thermal_monitor>  _thermal_monitor;
        std::shared_ptr<option> _thermal_toggle;

        std::function<void(const option&)> _recording_function = [](const option&) {};
    };
}
