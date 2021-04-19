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
        return (first._sequence_id == second._sequence_id) &&
            (first._exposure == second._exposure) &&
            (first._gain == second._gain);
    }

    class hdr_config
    {
    public:
        hdr_config(hw_monitor& hwm, std::shared_ptr<sensor_base> depth_ep,
            const option_range& exposure_range, const option_range& gain_range);


        float get(rs2_option option) const;
        void set(rs2_option option, float value, option_range range);
        bool is_config_in_process() const;

        bool is_enabled() const;

    private:
        bool is_hdr_id(int id) const;
        bool is_hdr_enabled_in_device(std::vector<byte>& result) const;
        bool is_current_subpreset_hdr(const std::vector<byte>& current_subpreset) const;
        bool configure_hdr_as_in_fw(const std::vector<byte>& current_subpreset);
        command prepare_hdr_sub_preset_command() const;
        std::vector<uint8_t> prepare_sub_preset_header() const;
        std::vector<uint8_t> prepare_sub_preset_frames_config() const;

        const int DEFAULT_HDR_ID = 0;
        const int DEFAULT_CURRENT_HDR_SEQUENCE_INDEX = -1;
        const int DEFAULT_HDR_SEQUENCE_SIZE = 2;

        // exposure value has been set to fit the 30 fps (default fps)
        const float PRE_ENABLE_HDR_EXPOSURE = 30000.f;

        const uint8_t CONTROL_ID_LASER = 0;
        const uint8_t CONTROL_ID_EXPOSURE = 1;
        const uint8_t CONTROL_ID_GAIN = 2;

        void set_options_to_be_restored_after_disable();
        void restore_options_after_disable();

        bool validate_config() const;
        bool send_sub_preset_to_fw();
        void disable();
        void set_id(float value);
        void set_sequence_size(float value);
        void set_sequence_index(float value);
        void set_enable_status(float value);
        void set_exposure(float value);
        void set_gain(float value);

        int _id;
        size_t _sequence_size;
        std::vector<hdr_params> _hdr_sequence_params;
        int _current_hdr_sequence_index;
        mutable bool _is_enabled;
        bool _is_config_in_process;
        bool _has_config_changed;
        bool _auto_exposure_to_be_restored;
        bool _emitter_on_off_to_be_restored;
        hw_monitor& _hwm;
        std::weak_ptr<sensor_base> _sensor;
        option_range _exposure_range;
        option_range _gain_range;
        bool _use_workaround;
        float _pre_hdr_exposure;
    };

}
