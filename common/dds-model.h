// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.
#pragma once

#include <librealsense2/rs.hpp>
#include <imgui.h>
#include <realsense_imgui.h>
#include <set>
#include <rsutils/type/ip-address.h>
#include <rsutils/string/hexdump.h>
#include <../third-party/rsutils/include/rsutils/type/eth-config.h>
#include <rsutils/json.h>
#include <rsutils/json-config.h>

uint32_t const GET_ETH_CONFIG = 0xBB;
uint32_t const SET_ETH_CONFIG = 0xBA;

bool const ACTUAL_VALUES = 0;
bool const DEFULT_VALUES = 1;

enum priority {
    ETH_FIRST,
    USB_FIRST,
    DYNAMIC
};

namespace rs2
{
    class ux_window;

    class dds_model
    {

    public:
        dds_model(rs2::device dev);

        void render_dds_config_window(ux_window& window, std::string& error_message);

        void open_dds_tool_window();

        void close_window() { ImGui::CloseCurrentPopup(); }

        eth_config get_eth_config(rs2::debug_protocol dev, bool defult_val);

        void set_eth_config(eth_config &new_config , std::string& error_message);

        bool supports_DDS();


    private:
        rs2::device _device;

        eth_config _defult_config;
        eth_config _current_config;
        eth_config _changed_config;

        bool _window_open;
        bool _no_reset;
        bool _set_defult;
        bool _dds_supported;

        void ipInputText(std::string label, rsutils::type::ip_address &ip);
        priority classifyPriority(link_priority &pr);
        bool check_DDS_support();
    };
}
