// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#include "dds-model.h"
#include "device-model.h"
#include "ux-window.h"
#include <rsutils/json.h>
#include <rsutils/json-config.h>
#include <rsutils/os/special-folder.h>
#include <rsutils/string/hexdump.h>
#include <imgui.h>
#include <realsense_imgui.h>

#include <iostream>
#include <fstream>

using namespace rs2;
using rsutils::json;
using rsutils::type::ip_address;

namespace rs2 {

    uint32_t const GET_ETH_CONFIG = 0xBB;
    uint32_t const SET_ETH_CONFIG = 0xBA;

    int const CURRENT_VALUES = 0;
    int const DEFULT_VALUES = 1;
}

dds_model::dds_model( rs2::device dev )
    : _device( dev )
    , _window_open( false )
    , _no_reset( false )
    , _set_defult( false )
    , _dds_supported( false )
{
    if( check_DDS_support() )
    {
        _defult_config = get_eth_config( DEFULT_VALUES );
        _current_config = get_eth_config( CURRENT_VALUES );
        _changed_config = _current_config;
        _dds_supported = true;
    }
}

eth_config dds_model::get_eth_config( int curr_or_default )
{
    rs2::debug_protocol dev( _device );
    auto cmd = dev.build_command( GET_ETH_CONFIG, curr_or_default );
    auto data = dev.send_and_receive_raw_data( cmd );
    int32_t const & code = *reinterpret_cast< int32_t const * >( data.data() );
    data.erase( data.begin(), data.begin() + sizeof( code ) );
    return eth_config( data );
}

void rs2::dds_model::set_eth_config( eth_config & new_config, std::string & error_message )
{
    rs2::debug_protocol hwm( _device );
    auto cmd = hwm.build_command( SET_ETH_CONFIG, 0, 0, 0, 0, new_config.build_command() );
    auto data = hwm.send_and_receive_raw_data( cmd );
    int32_t const & code = *reinterpret_cast< int32_t const * >( data.data() );
    if( data.size() != sizeof( code ) )
    {
        error_message = rsutils::string::from() << "Failed to change: bad response size " << data.size() << ' '
                                                << rsutils::string::hexdump( data.data(), data.size() );
        close_window();
    }
    if( code != SET_ETH_CONFIG )
    {
        error_message = rsutils::string::from() << "Failed to change: bad response " << code;
        close_window();
    }
    if( ! _no_reset )
    {
        close_window();
        _device.hardware_reset();
    }
}

bool rs2::dds_model::supports_DDS()
{
    return _dds_supported;
}

rs2::dds_model::priority rs2::dds_model::classifyPriority( link_priority & pr )
{
    if( pr == link_priority::usb_only || pr == link_priority::usb_first )
    {
        return priority::USB_FIRST;
    }
    else if( pr == link_priority::eth_first || pr == link_priority::eth_only )
    {
        return priority::ETH_FIRST;
    }
    return priority::DYNAMIC;
}

bool dds_model::check_DDS_support()
{
    if( _device.is< rs2::debug_protocol >() )
    {
        if( _device.supports( RS2_CAMERA_INFO_NAME ) )
        {
            std::string name = _device.get_info( RS2_CAMERA_INFO_NAME );
            if( name.find( "Recovery" ) != std::string::npos )
                return false; // Recovery devices don't support HWM.
        }

        try
        {
            auto dev = debug_protocol( _device );
            auto cmd = dev.build_command( GET_ETH_CONFIG, CURRENT_VALUES );
            auto data = dev.send_and_receive_raw_data( cmd );
            int32_t const & code = *reinterpret_cast< int32_t const * >( data.data() );
            if( code == GET_ETH_CONFIG )
                return true;
        }
        catch( ... )
        {
        }
    }

    return false;
}

void rs2::dds_model::ipInputText( std::string label, ip_address & ip )
{
    char buffer[16];
    std::string ip_str = ip.to_string();
    std::snprintf( buffer, sizeof( buffer ), "%s", ip_str.c_str() );
    std::string label_name = "##" + label;

    if( ImGui::InputText( label_name.c_str(), buffer, sizeof( buffer ) ) )
    {
        std::string new_ip_str( buffer );
        ip_address new_ip = ip_address( new_ip_str );
        if( new_ip.is_valid() )
        {
            ip = new_ip;
        }
        else
        {
            // Initialize the ImGui buffer with the current IP address in case of invalid input
            std::snprintf( buffer, sizeof( buffer ), "%s", ip.to_string().c_str() );
        }
    }
}

void dds_model::render_dds_config_window( ux_window & window, std::string & error_message )
{
    const auto window_name = "DDS Configuration";
    if( _window_open )
    {
        try
        {
            _current_config = get_eth_config( CURRENT_VALUES );
            _changed_config = _current_config;
            ImGui::OpenPopup( window_name );
        }
        catch( std::exception e )
        {
            error_message = e.what();
        }
        _window_open = false;
    }

    // Calculate window position and size
    const float w = 620;
    const float h = 500;
    const float x0 = std::max( window.width() - w, 0.f ) / 2;
    const float y0 = std::max( window.height() - h, 0.f ) / 2;
    ImGui::SetNextWindowPos( { x0, y0 } );
    ImGui::SetNextWindowSize( { w, h } );

    auto flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse
               | ImGuiWindowFlags_NoSavedSettings;

    ImGui::PushStyleColor( ImGuiCol_PopupBg, sensor_bg );
    ImGui::PushStyleColor( ImGuiCol_TextSelectedBg, light_grey );
    ImGui::PushStyleColor( ImGuiCol_Text, light_grey );
    ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 5, 5 ) );
    ImGui::PushStyleVar( ImGuiStyleVar_WindowRounding, 1 );
    ImGui::PushStyleColor( ImGuiCol_Button, button_color );
    ImGui::PushStyleColor( ImGuiCol_ButtonHovered, button_color + 0.1f );
    ImGui::PushStyleColor( ImGuiCol_ButtonActive, button_color + 0.1f );


    if( ImGui::BeginPopupModal( window_name, nullptr, flags ) )
    {
        if( error_message != "" )
            ImGui::CloseCurrentPopup();

        // Title
        const char * title_message = window_name;
        ImVec2 title_size = ImGui::CalcTextSize( title_message );
        float title_x = ( w - title_size.x - 10 ) / 2.0f;
        ImGui::SetCursorPos( { title_x, 10.0f } );
        ImGui::PushFont( window.get_large_font() );
        ImGui::PushStyleColor( ImGuiCol_Text, white );
        ImGui::Text( "%s", title_message );
        ImGui::PopStyleColor();
        ImGui::PopFont();
        ImGui::Separator();
        ImGui::SetCursorPosY( ImGui::GetCursorPosY() + 15 );

        // Main Scrollable Section
        ImGui::BeginChild( "MainContent", ImVec2( w - 10, h - 100 ), true );
        ImGui::PushItemWidth( 150.0f );

        // Connection Priority Section
        priority connection_priority = classifyPriority( _changed_config.link.priority );
        if( ImGui::CollapsingHeader( "Connection Priority" ) )
        {
            ImGui::Text( "Select connection priority:" );
            ImGui::RadioButton( "Ethernet First", reinterpret_cast< int * >( &connection_priority ), 0 );
            if( static_cast< int >( connection_priority ) == 0 )
            {
                ImGui::SameLine();
                ImGui::SetCursorPosX( ImGui::GetCursorPosX() + 50 );
                ImGui::Text( "Link Timeout (seconds)" );
                ImGui::SameLine();
                int tempTimeout = static_cast< int >( _changed_config.link.timeout );
                if( ImGui::InputInt( "##Link Timeout (seconds)", &tempTimeout ) )
                {
                    _changed_config.link.timeout = static_cast< uint16_t >( std::max( 0, tempTimeout ) );
                }
            }
            ImGui::RadioButton( "USB First", reinterpret_cast< int * >( &connection_priority ), 1 );
            ImGui::RadioButton( "Dynamic Priority", reinterpret_cast< int * >( &connection_priority ), 2 );
            switch( connection_priority )
            {
            case ETH_FIRST:
                _changed_config.link.priority = link_priority::eth_first;
                break;
            case USB_FIRST:
                _changed_config.link.priority = link_priority::usb_first;
                break;
            case DYNAMIC:
                _changed_config.link.priority
                    = _current_config.link.speed
                        ? link_priority::dynamic_eth_first
                        : link_priority::dynamic_usb_first;  // If link speed is not 0 than we are connected by Ethernet
                break;
            }
        }

        // Network Configuration Section
        if( ImGui::CollapsingHeader( "Network Configuration" ) )
        {
            ImGui::Checkbox( "Enable DHCP", &_changed_config.dhcp.on );
            if( ! _changed_config.dhcp.on )
            {
                ImGui::Text( "Static IP Address" );
                ImGui::SameLine();
                float textbox_align = ImGui::GetCursorPosX();
                ipInputText( "Static IP Address", _changed_config.configured.ip );
                ImGui::Text( "Subnet Mask" );
                ImGui::SameLine();
                ImGui::SetCursorPosX( textbox_align );
                bool maskStylePushed = false;
                ipInputText( "Subnet Mask", _changed_config.configured.netmask );
                ImGui::Text( "Gateway" );
                ImGui::SameLine();
                ImGui::SetCursorPosX( textbox_align );
                ipInputText( "Gateway", _changed_config.configured.gateway );
            }
            else
            {
                ImGui::Text( "DHCP Timeout (seconds)" );
                ImGui::SameLine();
                int tempTimeout = static_cast< int >( _changed_config.dhcp.timeout );
                if( ImGui::InputInt( "##DHCP Timeout (seconds)", &tempTimeout ) )
                {
                    _changed_config.dhcp.timeout = static_cast< uint16_t >( std::max( 0, tempTimeout ) );
                }
            }
        }

        ImGui::Text( "Domain ID" );
        ImGui::SameLine();
        if( ImGui::InputInt( "##Domain ID", &_changed_config.dds.domain_id ) )
        {
            if( _changed_config.dds.domain_id < 0 )
                _changed_config.dds.domain_id = 0;
            else if( _changed_config.dds.domain_id > 232 )
                _changed_config.dds.domain_id = 232;
        }
        ImGui::Checkbox( "No Reset after changes", &_no_reset );

        if( ImGui::Checkbox( "Load defult values", &_set_defult ) )
        {
            if( _set_defult )
                _changed_config = _defult_config;
            else
                _changed_config = _current_config;
        }

        ImGui::PopItemWidth();
        ImGui::EndChild();

        // window buttons
        float button_width = 115.0f;
        float spacing = 10.0f;
        float total_buttons_width = button_width * 4 + spacing * 2;
        float start_x = ( w - total_buttons_width ) / 2.0f;
        bool hasChanges = ( _changed_config != _current_config );

        ImGui::SetCursorPosY( ImGui::GetCursorPosY() + 8 );

        ImGui::SetCursorPosX( start_x );

        if( ImGui::Button( "Cancel", ImVec2( button_width, 25 ) ) )
        {
            close_window();
        }
        if( ImGui::IsItemHovered() )
        {
            window.link_hovered();
            RsImGui::CustomTooltip( "%s", "Close without saving any changes" );
        }
        ImGui::SameLine();
        if( ImGui::Button( "Factory Reset", ImVec2( button_width, 25 ) ) )
        {
            set_eth_config( _defult_config, error_message );
            close_window();
        }
        if( ImGui::IsItemHovered() )
        {
            window.link_hovered();
            RsImGui::CustomTooltip( "%s", "Reset settings back to defult values" );
        }
        ImGui::SameLine();
        RsImGui::RsImButton(
            [&]()
            {
                if( ImGui::ButtonEx( "Revert changes", ImVec2( button_width, 25 ) ) )
                {
                    _changed_config = _current_config;
                };
            },
            ! hasChanges );
        if( ImGui::IsItemHovered() )
        {
            window.link_hovered();
            RsImGui::CustomTooltip( "%s", "Revert to current configuration values" );
        }
        ImGui::SameLine();
        RsImGui::RsImButton(
            [&]()
            {
                if( ImGui::ButtonEx( "Apply", ImVec2( button_width, 25 ) ) )
                {
                    set_eth_config( _changed_config, error_message );
                    close_window();
                };
            },
            ! hasChanges );
        if( ImGui::IsItemHovered() )
        {
            window.link_hovered();
            RsImGui::CustomTooltip( "%s", "Apply changes" );
        }
        if( ImGui::BeginPopupModal( "No Changes Needed", NULL, ImGuiWindowFlags_AlwaysAutoResize ) )
        {
            ImGui::Text( "No changes were made to the configuration." );

            if( ImGui::Button( "OK", ImVec2( 100, 25 ) ) )
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        ImGui::EndPopup();
    }
    ImGui::PopStyleColor( 6 );
    ImGui::PopStyleVar( 2 );
}

void rs2::dds_model::open_dds_tool_window()
{
    _window_open = true;
}