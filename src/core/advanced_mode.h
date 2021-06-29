// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.
#pragma once

#include "ds5/ds5-private.h"
#include "hw-monitor.h"
#include "streaming.h"
#include "option.h"
#define RS400_ADVANCED_MODE_HPP
#include "ds5/advanced_mode/presets.h"
#include "../../include/librealsense2/h/rs_advanced_mode_command.h"
#include "serializable-interface.h"

#undef RS400_ADVANCED_MODE_HPP


typedef enum
{
    etDepthControl              = 0,
    etRsm                       = 1,
    etRauSupportVectorControl   = 2,
    etColorControl              = 3,
    etRauColorThresholdsControl = 4,
    etSloColorThresholdsControl = 5,
    etSloPenaltyControl         = 6,
    etHdad                      = 7,
    etColorCorrection           = 8,
    etDepthTableControl         = 9,
    etAEControl                 = 10,
    etCencusRadius9             = 11,
    etAFactor                   = 12,
    etLastAdvancedModeGroup     = 13,       // Must be last
}
EtAdvancedModeRegGroup;

namespace librealsense
{
    class ds5_color_sensor;

    template<class T>
    struct advanced_mode_traits;

    #define MAP_ADVANCED_MODE(T, E) template<> struct advanced_mode_traits<T> { static const EtAdvancedModeRegGroup group = E; }

    MAP_ADVANCED_MODE(STDepthControlGroup, etDepthControl);
    MAP_ADVANCED_MODE(STRsm, etRsm);
    MAP_ADVANCED_MODE(STRauSupportVectorControl, etRauSupportVectorControl);
    MAP_ADVANCED_MODE(STColorControl, etColorControl);
    MAP_ADVANCED_MODE(STRauColorThresholdsControl, etRauColorThresholdsControl);
    MAP_ADVANCED_MODE(STSloColorThresholdsControl, etSloColorThresholdsControl);
    MAP_ADVANCED_MODE(STSloPenaltyControl, etSloPenaltyControl);
    MAP_ADVANCED_MODE(STHdad, etHdad);
    MAP_ADVANCED_MODE(STColorCorrection, etColorCorrection);
    MAP_ADVANCED_MODE(STDepthTableControl, etDepthTableControl);
    MAP_ADVANCED_MODE(STAEControl, etAEControl);
    MAP_ADVANCED_MODE(STCensusRadius, etCencusRadius9);
    MAP_ADVANCED_MODE(STAFactor, etAFactor);


    class ds5_advanced_mode_interface : public serializable_interface, public recordable<ds5_advanced_mode_interface>
    {
    public:
        virtual bool is_enabled() const = 0;

        virtual void toggle_advanced_mode(bool enable) = 0;

        virtual void apply_preset(const std::vector<platform::stream_profile>& configuration,
                                  rs2_rs400_visual_preset preset, uint16_t device_pid,
                                  const firmware_version& fw_version) = 0;

        virtual void get_depth_control_group(STDepthControlGroup* ptr, int mode = 0) const = 0;
        virtual void get_rsm(STRsm* ptr, int mode = 0) const = 0;
        virtual void get_rau_support_vector_control(STRauSupportVectorControl* ptr, int mode = 0) const = 0;
        virtual void get_color_control(STColorControl* ptr, int mode = 0) const = 0;
        virtual void get_rau_color_thresholds_control(STRauColorThresholdsControl* ptr, int mode = 0) const = 0;
        virtual void get_slo_color_thresholds_control(STSloColorThresholdsControl* ptr, int mode = 0) const = 0;
        virtual void get_slo_penalty_control(STSloPenaltyControl* ptr, int mode = 0) const = 0;
        virtual void get_hdad(STHdad* ptr, int mode = 0) const = 0;
        virtual void get_color_correction(STColorCorrection* ptr, int mode = 0) const = 0;
        virtual void get_depth_table_control(STDepthTableControl* ptr, int mode = 0) const = 0;
        virtual void get_ae_control(STAEControl* ptr, int mode = 0) const = 0;
        virtual void get_census_radius(STCensusRadius* ptr, int mode = 0) const = 0;
        virtual void get_amp_factor(STAFactor* ptr, int mode = 0) const = 0;

        virtual void set_depth_control_group(const STDepthControlGroup& val) = 0;
        virtual void set_rsm(const STRsm& val) = 0;
        virtual void set_rau_support_vector_control(const STRauSupportVectorControl& val) = 0;
        virtual void set_color_control(const STColorControl& val) = 0;
        virtual void set_rau_color_thresholds_control(const STRauColorThresholdsControl& val) = 0;
        virtual void set_slo_color_thresholds_control(const STSloColorThresholdsControl& val) = 0;
        virtual void set_slo_penalty_control(const STSloPenaltyControl& val) = 0;
        virtual void set_hdad(const STHdad& val) = 0;
        virtual void set_color_correction(const STColorCorrection& val) = 0;
        virtual void set_depth_table_control(const STDepthTableControl& val) = 0;
        virtual void set_ae_control(const STAEControl& val) = 0;
        virtual void set_census_radius(const STCensusRadius& val) = 0;
        virtual void set_amp_factor(const STAFactor& val) = 0;

        virtual ~ds5_advanced_mode_interface() = default;
    };

    MAP_EXTENSION(RS2_EXTENSION_ADVANCED_MODE, librealsense::ds5_advanced_mode_interface);

    class advanced_mode_preset_option;

    class ds5_advanced_mode_base : public ds5_advanced_mode_interface
    {
    public:
        explicit ds5_advanced_mode_base(std::shared_ptr<hw_monitor> hwm,
            synthetic_sensor& depth_sensor);

        void create_snapshot(std::shared_ptr<ds5_advanced_mode_interface>& snapshot) const override {};
        void enable_recording(std::function<void(const ds5_advanced_mode_interface&)> recording_function) override {};

        virtual ~ds5_advanced_mode_base() = default;

        bool is_enabled() const override;
        void toggle_advanced_mode(bool enable) override;
        void apply_preset(const std::vector<platform::stream_profile>& configuration,
                          rs2_rs400_visual_preset preset, uint16_t device_pid,
                          const firmware_version& fw_version) override;

        void get_depth_control_group(STDepthControlGroup* ptr, int mode = 0) const override;
        void get_rsm(STRsm* ptr, int mode = 0) const override;
        void get_rau_support_vector_control(STRauSupportVectorControl* ptr, int mode = 0) const override;
        void get_color_control(STColorControl* ptr, int mode = 0) const override;
        void get_rau_color_thresholds_control(STRauColorThresholdsControl* ptr, int mode = 0) const override;
        void get_slo_color_thresholds_control(STSloColorThresholdsControl* ptr, int mode = 0) const override;
        void get_slo_penalty_control(STSloPenaltyControl* ptr, int mode = 0) const override;
        void get_hdad(STHdad* ptr, int mode = 0) const override;
        void get_color_correction(STColorCorrection* ptr, int mode = 0) const override;
        void get_depth_table_control(STDepthTableControl* ptr, int mode = 0) const override;
        void get_ae_control(STAEControl* ptr, int mode = 0) const override;
        void get_census_radius(STCensusRadius* ptr, int mode = 0) const override;
        void get_amp_factor(STAFactor* ptr, int mode = 0) const override;

        void set_depth_control_group(const STDepthControlGroup& val) override;
        void set_rsm(const STRsm& val) override;
        void set_rau_support_vector_control(const STRauSupportVectorControl& val) override;
        void set_color_control(const STColorControl& val) override;
        void set_rau_color_thresholds_control(const STRauColorThresholdsControl& val) override;
        void set_slo_color_thresholds_control(const STSloColorThresholdsControl& val) override;
        void set_slo_penalty_control(const STSloPenaltyControl& val) override;
        void set_hdad(const STHdad& val) override;
        void set_color_correction(const STColorCorrection& val) override;
        void set_depth_table_control(const STDepthTableControl& val) override;
        void set_ae_control(const STAEControl& val) override;
        void set_census_radius(const STCensusRadius& val) override;
        void set_amp_factor(const STAFactor& val) override;

        std::vector<uint8_t> serialize_json() const override;
        void load_json(const std::string& json_content) override;

        static const uint16_t HW_MONITOR_COMMAND_SIZE = 1000;
        static const uint16_t HW_MONITOR_BUFFER_SIZE = 1024;

    private:
        friend class auto_calibrated;
        void set_exposure(synthetic_sensor& sensor, const exposure_control& val);
        void set_auto_exposure(synthetic_sensor& sensor, const auto_exposure_control& val);
        void get_exposure(synthetic_sensor& sensor, exposure_control* ptr) const;
        void get_auto_exposure(synthetic_sensor& sensor, auto_exposure_control* ptr) const;

        void get_laser_power(laser_power_control* ptr) const;
        void get_laser_state(laser_state_control* ptr) const;
        void get_depth_exposure(exposure_control* ptr) const;
        void get_depth_auto_exposure(auto_exposure_control* ptr) const;
        void get_depth_gain(gain_control* ptr) const;
        void get_depth_auto_white_balance(auto_white_balance_control* ptr) const;
        void get_color_exposure(exposure_control* ptr) const;
        void get_color_auto_exposure(auto_exposure_control* ptr) const;
        void get_color_backlight_compensation(backlight_compensation_control* ptr) const;
        void get_color_brightness(brightness_control* ptr) const;
        void get_color_contrast(contrast_control* ptr) const;
        void get_color_gain(gain_control* ptr) const;
        void get_color_gamma(gamma_control* ptr) const;
        void get_color_hue(hue_control* ptr) const;
        void get_color_saturation(saturation_control* ptr) const;
        void get_color_sharpness(sharpness_control* ptr) const;
        void get_color_white_balance(white_balance_control* ptr) const;
        void get_color_auto_white_balance(auto_white_balance_control* ptr) const;
        void get_color_power_line_frequency(power_line_frequency_control* ptr) const;

        void set_laser_power(const laser_power_control& val);
        void set_laser_state(const laser_state_control& val);
        void set_depth_exposure(const exposure_control& val);
        void set_depth_auto_exposure(const auto_exposure_control& val);
        void set_depth_gain(const gain_control& val);
        void set_depth_auto_white_balance(const auto_white_balance_control& val);
        void set_color_exposure(const exposure_control& val);
        void set_color_auto_exposure(const auto_exposure_control& val);
        void set_color_backlight_compensation(const backlight_compensation_control& val);
        void set_color_brightness(const brightness_control& val);
        void set_color_contrast(const contrast_control& val);
        void set_color_gain(const gain_control& val);
        void set_color_gamma(const gamma_control& val);
        void set_color_hue(const hue_control& val);
        void set_color_saturation(const saturation_control& val);
        void set_color_sharpness(const sharpness_control& val);
        void set_color_white_balance(const white_balance_control& val);
        void set_color_auto_white_balance(const auto_white_balance_control& val);
        void set_color_power_line_frequency(const power_line_frequency_control& val);

        bool supports_option(const synthetic_sensor& sensor, rs2_option opt) const;

        std::shared_ptr<hw_monitor> _hw_monitor;
        synthetic_sensor& _depth_sensor;
        lazy<ds5_color_sensor*> _color_sensor;
        lazy<bool> _enabled;
        std::shared_ptr<advanced_mode_preset_option> _preset_opt;
        lazy<bool> _rgb_exposure_gain_bind;
        lazy<bool> _amplitude_factor_support;

        preset get_all() const;
        void set_all(const preset& p);

        std::vector<uint8_t> send_receive(const std::vector<uint8_t>& input) const;

        template<class T>
        void set(const T& strct, EtAdvancedModeRegGroup cmd) const
        {
            auto ptr = (uint8_t*)(&strct);
            std::vector<uint8_t> data(ptr, ptr + sizeof(T));

            assert_no_error(ds::fw_cmd::SET_ADV,
                send_receive(encode_command(ds::fw_cmd::SET_ADV, static_cast<uint32_t>(cmd), 0, 0, 0, data)));
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }

        template<class T>
        T get(EtAdvancedModeRegGroup cmd, T* ptr = static_cast<T*>(nullptr), int mode = 0) const
        {
            T res;
            auto data = assert_no_error(ds::fw_cmd::GET_ADV,
                send_receive(encode_command(ds::fw_cmd::GET_ADV,
                static_cast<uint32_t>(cmd), mode)));
            if (data.size() < sizeof(T))
            {
                throw std::runtime_error("The camera returned invalid sized result!");
            }
            res = *reinterpret_cast<T*>(data.data());
            return res;
        }

        static uint32_t pack(uint8_t c0, uint8_t c1, uint8_t c2, uint8_t c3);

        static std::vector<uint8_t> assert_no_error(ds::fw_cmd opcode, const std::vector<uint8_t>& results);

        std::vector<uint8_t> encode_command(ds::fw_cmd opcode,
                                            uint32_t p1 = 0,
                                            uint32_t p2 = 0,
                                            uint32_t p3 = 0,
                                            uint32_t p4 = 0,
                                            std::vector<uint8_t> data = std::vector<uint8_t>()) const;


    };


    class advanced_mode_preset_option : public option_base
    {
    public:
        advanced_mode_preset_option(ds5_advanced_mode_base& advanced, synthetic_sensor& ep,
                                    const option_range& opt_range);

        static rs2_rs400_visual_preset to_preset(float x);
        void set(float value) override;
        float query() const override;
        bool is_enabled() const override;
        const char* get_description() const override;
        const char* get_value_description(float val) const override;

    private:
        uint16_t get_device_pid(const uvc_sensor& sensor) const;
        firmware_version get_firmware_version(const uvc_sensor& sensor) const;

        std::mutex _mtx;
        synthetic_sensor& _ep;
        ds5_advanced_mode_base& _advanced;
        rs2_rs400_visual_preset _last_preset;
    };
}
