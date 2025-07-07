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
struct hdr_config
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
        int iterations;
        std::vector< control_item > controls;

        preset_item();
    };

    struct hdr_preset
    {
        std::string id;
        int iterations;
        std::vector< preset_item > items;

        hdr_preset();
    };

    hdr_preset _hdr_preset;

    std::string to_json() const;
    void from_json( const std::string & json_str );

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
    void apply_hdr_config();

private:
    rs2::device _device;
    bool _window_open;
    bool _hdr_supported;
    bool _set_default;

    hdr_config _current_config;
    hdr_config _changed_config;
    hdr_config _default_config;

    void close_window();
    bool check_HDR_support();
    void initialize_default_config();

    void render_preset_item( hdr_config::preset_item & item, int item_index );
    void render_control_item( hdr_config::control_item & control, int control_index );
};

}