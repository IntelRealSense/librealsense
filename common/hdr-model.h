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
        int depth_man_exp;

        control_item();
        control_item( int gain, int exp );
    };

    struct preset_item
    {
        int iterations;  // current item will be applied for this many iterations
        control_item controls;

        preset_item();

        bool operator==( const preset_item & other ) const
        {
            if (controls.depth_gain != other.controls.depth_gain)
                return false;
            if (controls.depth_man_exp != other.controls.depth_man_exp)
                return false;
            return iterations == other.iterations;
        }
    };


    std::string id;
    int iterations;  // the preset will be applied for this many iterations, if 0 it will be infinite
    std::vector< preset_item > items;

    hdr_preset();

    std::string to_json() const;
    void from_json( const std::string & json_str );

    bool operator==( const hdr_preset& other ) const
    {
        return id == other.id &&
               iterations == other.iterations &&
               items == other.items;
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
    bool _set_default;
    option_range _exp_range;
    option_range _gain_range;
    bool exp_edit_mode = false;
    bool gain_edit_mode = false;

    hdr_preset _current_config;
    hdr_preset _changed_config;
    hdr_preset _default_config;

    void close_window();
    bool check_HDR_support();
    void initialize_default_config();

    void render_preset_item( hdr_preset::preset_item & item, int item_index );
    void render_control_item( hdr_preset::control_item & control);
};

}