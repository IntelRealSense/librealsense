/* License: Apache 2.0. See LICENSE file in root directory.
Copyright(c) 2020 Intel Corporation. All Rights Reserved. */


#pragma once

#include <vector>
#include "hw-monitor.h"

namespace librealsense
{
    struct hdr_params {
        int _sequence_id;
        float _exposure;
        float _gain;

        hdr_params();
        hdr_params(int sequence_id, float exposure, float gain);

        hdr_params& operator=(const hdr_params& other);
    };

    inline bool operator==(const hdr_params& first, const hdr_params& second)
    {
        return (first._exposure == second._exposure) && (first._gain == second._gain);
    }
    
    class hdr_config
    {
    public:
        hdr_config(hw_monitor& hwm, sensor_base* depth_ep, option_range exposure_range, option_range gain_range);

        float get(rs2_option option) const;
        void set(rs2_option option, float value, option_range range);
        bool is_config_in_process() const;


    private:
        command prepare_hdr_sub_preset_command() const;
        std::vector<uint8_t> prepare_sub_preset_header() const;
        std::vector<uint8_t> prepare_sub_preset_frames_config() const;

        typedef enum control_id
        {
            depth_laser_mode = 0,
            depth_manual_exposure = 1,
            depth_gain = 2
        }control_id;

        bool validate_config() const;
        void send_sub_preset_to_fw();
        void disable();
        void set_sequence_size(float value);
        void set_sequence_index(float value);
        void set_enable_status(float value);
        void set_exposure(float value, option_range range);
        void set_gain(float value);
        void reset_to_default();

        size_t _sequence_size;
        std::vector<hdr_params> _hdr_sequence_params;
        int _current_hdr_sequence_index;
        bool _is_enabled;
        bool _is_config_in_process;
        bool _has_config_changed;
        hw_monitor& _hwm;
        sensor_base* _sensor;
        option_range _exposure_range;
        option_range _gain_range;
    };

    

    

}