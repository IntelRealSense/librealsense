// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2025 Intel Corporation. All Rights Reserved.

#pragma once

#include <vector>
#include <string>
#include <algorithm>
#include <rsutils/json.h>
#include <librealsense2/rs.hpp>
#include "ux-window.h"

namespace rs2 {
struct hdr_preset
{
    struct control_item
    {
        int depth_gain;
        int depth_exp;

        int delta_gain;
        int delta_exp;

        control_item();
        control_item( int gain, int exp );
        control_item( int gain, int exp, int delta_gain, int delta_exp );
    };

    struct preset_item
    {
        int iterations;  // controls will be applied for this many iterations
        control_item controls;

        preset_item();
    };


    std::string id;
    int iterations;  // the preset will be applied for this many iterations, if 0 it will be infinite
    std::vector< preset_item > items;
    bool control_type_auto; // in auto mode, exposure and gain are given in deltas, which are applied on the auto exposure values

    hdr_preset();

    std::string to_json() const;
    void from_json( const std::string & json_str );

    bool operator==( const hdr_preset& other ) const
    {
        if( id != other.id || iterations != other.iterations 
            || items.size() != other.items.size() || control_type_auto != other.control_type_auto)
            return false;
        for (size_t i = 0; i < items.size(); i++)
        {
            if (items[i].iterations != other.items[i].iterations)
                return false;

            auto & ctrls = items[i].controls;
            auto & other_ctrls = other.items[i].controls;
            if (control_type_auto)
            {
                if ( ctrls.delta_exp != other_ctrls.delta_exp || ctrls.delta_gain != other_ctrls.delta_gain )
                    return false;
            }
            else
            {
                if ( ctrls.depth_exp != other_ctrls.depth_exp || ctrls.depth_gain != other_ctrls.depth_gain )
                    return false;
            }
        }
        return true;
    }

};

class hdr_model
{
public:
    explicit hdr_model( rs2::device dev );

    void render_hdr_config_window( ux_window & window, std::string & error_message );
    void open_hdr_tool_window();
    bool supports_HDR() const;

    void load_hdr_config_from_file( const std::string & filename );
    void save_hdr_config_to_file( const std::string & filename );
    void load_hdr_config_from_device();
    void apply_hdr_config();

private:
    rs2::device _device;
    bool _window_open;
    bool _hdr_supported;
    bool _is_auto = false;
    option_range _exp_range;
    option_range _gain_range;

    hdr_preset _current_config;  // the current configuration in the device
    hdr_preset _changed_config;  // the configuration that is being edited in the UI
    hdr_preset _default_config;

    void close_window();
    bool check_HDR_support();
    void initialize_default_config();

    void render_preset_item( hdr_preset::preset_item & item, int item_index );
    void render_control_item( hdr_preset::control_item & control);
};

}