// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2025 Intel Corporation. All Rights Reserved.

#include <glad/glad.h>
#include "hdr-model.h"
#include "device-model.h"
#include "ux-window.h"
#include <rsutils/os/special-folder.h>
#include <rsutils/string/hexdump.h>
#include <imgui.h>
#include <realsense_imgui.h>
#include <fstream>
#include <stdexcept>
#include <os.h>
using rsutils::json;

namespace rs2 {
hdr_config::control_item::control_item()
    : control_item( 1, 1 )
{
}

hdr_config::control_item::control_item( int gain, int exp )
    : depth_gain( gain )
    , depth_man_exp( exp )
{
}

hdr_config::preset_item::preset_item()
    : iterations( 1 )
{
}

hdr_config::hdr_preset::hdr_preset()
    : id( "0" )
    , iterations( 0 )
{
}

std::string hdr_config::to_json() const
{
    json j;
    auto & hp = j["hdr-preset"];
    hp["id"] = _hdr_preset.id;
    hp["iterations"] = std::to_string( _hdr_preset.iterations );

    hp["items"] = json::array();
    for( auto const & item : _hdr_preset.items )
    {
        json item_j;
        item_j["iterations"] = std::to_string( item.iterations );
        item_j["controls"] = json::array();

        if( ! item.controls.empty() )
        {
            auto const & ctrl = item.controls[0];
            json gain_j;
            gain_j["depth-gain"] = std::to_string( ctrl.depth_gain );
            json exp_j;
            exp_j["depth-exposure"] = std::to_string( ctrl.depth_man_exp );
            item_j["controls"].push_back( gain_j );
            item_j["controls"].push_back( exp_j );
        }
        hp["items"].push_back( item_j );
    }

    return j.dump( 4 );
}

void hdr_config::from_json( const std::string & json_str )
{
    auto j = json::parse( json_str );
    if( ! j.contains( "hdr-preset" ) )
        throw std::runtime_error( "Missing hdr-preset" );

    auto & hp = j["hdr-preset"];
    _hdr_preset.id = hp.value( "id", "0" );
    _hdr_preset.iterations = std::stoi( hp.value( "iterations", "0" ) );

    _hdr_preset.items.clear();
    for( auto & item_j : hp["items"] )
    {
        preset_item item;
        item.iterations = std::stoi( item_j.value( "iterations", "1" ) );

        const auto & ctrls = item_j["controls"];
        if( ctrls.size() >= 2 )
        {
            control_item ctrl;
            if( ctrls[0].contains( "depth-gain" ) )
                ctrl.depth_gain = std::stoi( ctrls[0]["depth-gain"].get< std::string >() );
            if( ctrls[1].contains( "depth-exposure" ) )
                ctrl.depth_man_exp = std::stoi( ctrls[1]["depth-exposure"].get< std::string >() );

            item.controls.push_back( ctrl );
        }
        _hdr_preset.items.push_back( std::move( item ) );
    }
}

hdr_model::hdr_model( rs2::device dev )
    : _device( dev )
    , _window_open( false )
    , _hdr_supported( false )
    , _set_default( false )
{
    _hdr_supported = check_HDR_support();
    if( _hdr_supported )
    {
        initialize_default_config();
        _current_config = _default_config;
        _changed_config = _current_config;
    }
}

bool hdr_model::check_HDR_support()
{
    try
    {
        if( ! _device.is< rs2::serializable_device >() )
            return false;
        auto serializable = _device.as< rs2::serializable_device >();
        return ! serializable.serialize_json().empty();
    }
    catch( ... )
    {
    }
    return false;
}

void hdr_model::initialize_default_config()
{
    _default_config._hdr_preset.id = "0";
    _default_config._hdr_preset.iterations = 0;

    const std::vector< std::pair< int, int > > defaults
        = { { 1, 1 }, { 32, 1000 }, { 128, 100000 }, { 2, 2 }, { 232, 200 }, { 2128, 20000 } };
    for( auto const & p : defaults )
    {
        hdr_config::preset_item it;
        it.iterations = 1;
        it.controls.emplace_back( p.first, p.second );
        _default_config._hdr_preset.items.push_back( it );
    }
}

bool hdr_model::supports_HDR() const
{
    return _hdr_supported;
}

void hdr_model::load_hdr_config_from_file( const std::string & filename )
{
    std::ifstream file( filename );
    if( ! file )
        throw std::runtime_error( "Cannot open file " + filename );
    std::string content( ( std::istreambuf_iterator< char >( file ) ), {} );
    _changed_config.from_json( content );
}

void hdr_model::save_hdr_config_to_file( const std::string & filename )
{
    std::ofstream file( filename );
    if( ! file )
        throw std::runtime_error( "Cannot write file " + filename );
    file << _current_config.to_json();
}

void hdr_model::apply_hdr_config()
{
    if( ! _device.is< rs2::serializable_device >() )
        throw std::runtime_error( "Device not serializable" );
    auto serializable = _device.as< rs2::serializable_device >();
    serializable.load_json( _changed_config.to_json() );
    _current_config = _changed_config;
}

void hdr_model::render_control_item( hdr_config::control_item & control, int /*idx*/ )
{
    ImGui::PushID( &control );

    ImGui::Text( "Depth Gain:" );
    ImGui::SameLine();
    ImGui::SetNextItemWidth( 100 );
    int g = control.depth_gain;
    if( ImGui::InputInt( "##gain", &g ) )
        control.depth_gain = std::max( 1, std::min( g, 16384 ) );

    ImGui::Text( "Depth Exposure:" );
    ImGui::SameLine();
    ImGui::SetNextItemWidth( 140 );
    int e = control.depth_man_exp;
    if( ImGui::InputInt( "##exp", &e ) )
        control.depth_man_exp = std::max( 1, std::min( e, 200000 ) );

    ImGui::PopID();
}

void hdr_model::render_preset_item( hdr_config::preset_item & item, int idx )
{
    ImGui::PushID( idx );
    std::string hdr = "Frame " + std::to_string( idx + 1 );
    if( ImGui::CollapsingHeader( hdr.c_str() ) )
    {
        ImGui::Text( "Iterations:" );
        ImGui::SameLine();
        ImGui::SetNextItemWidth( 100 );
        int it = item.iterations;
        if( ImGui::InputInt( "##it", &it ) )
            item.iterations = std::max( 1, it );

        ImGui::Separator();
        ImGui::Text( "controls:" );
        if( ! item.controls.empty() )
            render_control_item( item.controls[0], 0 );
    }
    ImGui::PopID();
}

void hdr_model::render_hdr_config_window( ux_window & window, std::string & error_message )
{
    if( ! _window_open || ! _hdr_supported )
        return;

    ImGui::SetNextWindowSize( { 680, 600 }, ImGuiCond_FirstUseEver );
    if( ! ImGui::Begin( "HDR Configuration", &_window_open, ImGuiWindowFlags_NoCollapse ) )
    {
        ImGui::End();
        return;
    }

    if( ! error_message.empty() )
        ImGui::TextColored( { 1, 0.3f, 0.3f, 1 }, "%s", error_message.c_str() );
    ImGui::Separator();

    ImGui::TextColored( ImVec4( 1.0f, 1.0f, 0.5f, 1.0f ), "SubPreset Configuration" );
    ImGui::Separator();

    ImGui::Text( "Preset ID:" );
    ImGui::SameLine();
    ImGui::SetNextItemWidth( 150 );
    char buf[32];
    strncpy( buf, _changed_config._hdr_preset.id.c_str(), 31 );
    buf[31] = '\0';
    if( ImGui::InputText( "##id", buf, 32 ) )
        _changed_config._hdr_preset.id = buf;

    ImGui::Text( "Global Iterations:" );
    ImGui::SameLine();
    int gi = _changed_config._hdr_preset.iterations;
    ImGui::SetNextItemWidth( 150 );
    if( ImGui::InputInt( "##gi", &gi ) )
        _changed_config._hdr_preset.iterations = std::max( 0, gi );

    ImGui::Spacing();
    ImGui::TextColored( ImVec4( 1.0f, 1.0f, 0.5f, 1.0f ), "Preset Items" );
    ImGui::Separator();

    for( size_t i = 0; i < _changed_config._hdr_preset.items.size(); ++i )
        render_preset_item( _changed_config._hdr_preset.items[i], int( i ) );

    ImGui::Spacing();
    ImGui::TextColored( ImVec4( 1.0f, 1.0f, 0.5f, 1.0f ), "Load/Save Configuration" );
    ImGui::Separator();

    if( ImGui::Button( "Load from File" ) )
    {
        auto ret = file_dialog_open( open_file, "JavaScript Object Notation (JSON)\0*.json\0", nullptr, nullptr );
        if( ret )
        {
            try
            {
                load_hdr_config_from_file( ret );
            }
            catch( const std::exception & e )
            {
                error_message = e.what();
            }
        }
    }
    ImGui::SameLine();
    if( ImGui::Button( "Save to File" ) )
    {
        auto ret = file_dialog_open( save_file, "JavaScript Object Notation (JSON)\0*.json\0", nullptr, nullptr );
        if( ret )
        {
            try
            {
                save_hdr_config_to_file( ret );
            }
            catch( const std::exception & e )
            {
                error_message = e.what();
            }
        }
    }

    ImGui::Separator();
    ImGui::PushStyleVar( ImGuiStyleVar_FrameRounding, 4.0f );
    ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, ImVec2( 8, 4 ) );

    if( ImGui::Button( "Apply", ImVec2( 100, 0 ) ) )
    {
        try
        {
            apply_hdr_config();
            _set_default = false;
            _window_open = false;
            error_message.clear();
        }
        catch( const std::exception & e )
        {
            error_message = e.what();
        }
    }

    ImGui::SameLine();

    if( ImGui::Button( "Cancel", ImVec2( 100, 0 ) ) )
    {
        close_window();
    }

    ImGui::PopStyleVar( 2 );

    ImGui::End();
}


void hdr_model::open_hdr_tool_window()
{
    _window_open = true;
    _changed_config = _current_config;
}

void hdr_model::close_window()
{
    _window_open = false;
}

}
