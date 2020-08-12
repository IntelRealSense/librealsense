/* License: Apache 2.0. See LICENSE file in root directory.
Copyright(c) 2020 Intel Corporation. All Rights Reserved. */


#pragma once

#include <vector>
#include "hw-monitor.h"

namespace librealsense
{
    struct hdr_params {
        const int _sequence_id;
        float _exposure;
        float _gain;

        hdr_params();
        hdr_params(int sequence_id, float exposure, float gain);
    };

    inline bool operator==(const hdr_params& first, const hdr_params& second)
    {
        return (first._exposure == second._exposure) && (first._gain == second._gain);
    }
    
    class hdr_config
    {
    public:
        hdr_config(hw_monitor& hwm, sensor_base* depth_ep);

        float get(rs2_option option) const;
        void set(rs2_option option, float value, option_range range);
        bool is_config_in_process() const;


    private:
        command prepare_hdr_sub_preset_command();
        bool validate_config() const;
        void enable();
        void disable();
        void set_sequence_size(float value);
        void set_relative_mode(float value);
        void set_sequence_index(float value);
        void set_is_active_status(float value);
        void set_exposure(float value, option_range range);
        void set_gain(float value);


        size_t _sequence_size;
        std::vector<hdr_params> _hdr_sequence_params;
        size_t _current_hdr_sequence_index;
        bool _relative_mode;
        bool _is_enabled;
        bool _is_config_in_process;
        hw_monitor& _hwm;
        sensor_base* _sensor;
    };

    

    

}