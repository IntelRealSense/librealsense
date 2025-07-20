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
hdr_preset::control_item::control_item()
    : control_item( 1, 1 )
{
}

hdr_preset::control_item::control_item( int gain, int exp )
    : depth_gain( gain )
    , depth_man_exp( exp )
{
}

hdr_preset::preset_item::preset_item()
    : iterations( 1 )
{
}

hdr_preset::hdr_preset::hdr_preset()
    : id( "0" )
    , iterations( 0 )
{
}

std::string hdr_preset::to_json() const
{
    json j;
    auto & hp = j["hdr-preset"];
    hp["id"] = id;
    hp["iterations"] = std::to_string( iterations );

    hp["items"] = json::array();
    for( auto const & item : items )
    {
        json item_j;
        item_j["iterations"] = std::to_string( item.iterations );
        item_j["controls"] = json::array();

        auto const& ctrl = item.controls;
        json gain_j;
        gain_j["depth-gain"] = std::to_string(ctrl.depth_gain);
        json exp_j;
        exp_j["depth-exposure"] = std::to_string(ctrl.depth_man_exp);
        item_j["controls"].push_back(gain_j);
        item_j["controls"].push_back(exp_j);

        hp["items"].push_back( item_j );
    }

    return j.dump( 4 );
}

void hdr_preset::from_json( const std::string & json_str )
{
    auto j = json::parse( json_str );
    if( ! j.contains( "hdr-preset" ) )
    {
        // no hdr preset in the JSON, initialize with default values
        *this = hdr_preset();
        return;
    }

    auto & hp = j["hdr-preset"];
    id = hp.value( "id", "0" );
    iterations = std::stoi( hp.value( "iterations", "0" ) );

    items.clear();
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
            item.controls = ctrl;
        }
        items.push_back( std::move( item ) );
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
        _exp_range = _device.first<rs2::depth_sensor>().get_option_range(RS2_OPTION_EXPOSURE);
        _gain_range = _device.first< rs2::depth_sensor >().get_option_range(RS2_OPTION_GAIN);

        initialize_default_config();
        load_hdr_config_from_device();
        _changed_config = _current_config;
        if( _changed_config.items.empty() )
        {
            // if the device does not have a preset, use the default one
            _changed_config = _default_config;
        }
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
    _default_config.id = "0";
    _default_config.iterations = 0;

    // pairs of gain and exposure
    const std::vector< std::pair< int, int > > defaults = { { 16, (int)_exp_range.min }, { 16, 32000 } };
    for (auto const& p : defaults)
    {
        hdr_preset::preset_item it;
        it.iterations = 1;
        it.controls = hdr_preset::control_item( p.first, p.second );
        _default_config.items.push_back( it );
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

void hdr_model::load_hdr_config_from_device()
{
    if (!_device.is< rs2::serializable_device >())
        throw std::runtime_error("Device not serializable");
    auto serializable = _device.as< rs2::serializable_device >();
    std::string hdr_config = serializable.serialize_json();
    _current_config.from_json(hdr_config);
}

void hdr_model::apply_hdr_config()
{
    if( ! _device.is< rs2::serializable_device >() )
        throw std::runtime_error( "Device not serializable" );

    bool configs_same = _changed_config == _current_config;
    auto depth_sensor = _device.first< rs2::depth_sensor >();
    auto hdr_enabled = depth_sensor.get_option(RS2_OPTION_HDR_ENABLED);
    if (configs_same && hdr_enabled) // no changes to apply
        return;

    // disable HDR before applying new config
    if (hdr_enabled)
    {
        depth_sensor.set_option(RS2_OPTION_HDR_ENABLED, 0.0f);  
    }

    auto serializable = _device.as< rs2::serializable_device >();
    serializable.load_json( _changed_config.to_json() );
    _current_config = _changed_config;
}

void hdr_model::render_control_item( hdr_preset::control_item & control)
{
    ImGui::PushID( &control );

    ImGui::Text( "Depth Gain:" );
    ImGui::SameLine();
    ImGui::SetNextItemWidth( 140 );
    int g = control.depth_gain;
    if (ImGui::InputInt("##gain", &g, 0, 0))
        control.depth_gain = (int)clamp((float)g, _gain_range.min, _gain_range.max);

    ImGui::Text( "Depth Exposure:" );
    ImGui::SameLine();
    ImGui::SetNextItemWidth( 140 );
    int e = control.depth_man_exp;
    if (ImGui::InputInt("##exp", &e, 0, 0))
        control.depth_man_exp = (int)clamp((float)e, _exp_range.min, _exp_range.max);

    ImGui::PopID();
}

void hdr_model::render_preset_item( hdr_preset::preset_item & item, int idx )
{
    ImGui::PushID( idx );
    std::string hdr = "Preset Item " + std::to_string( idx + 1 );
    if( ImGui::CollapsingHeader( hdr.c_str() ) )
    {
        ImGui::Text( "Iterations:" );
        if (ImGui::IsItemHovered())
            RsImGui::CustomTooltip("Number of consequtive frames to be received with the configuration in this preset item per global iteration");
        ImGui::SameLine();
        ImGui::SetNextItemWidth( 100 );
        int it = item.iterations;
        if( ImGui::InputInt( "##it", &it ) )
            item.iterations = std::max( 1, it );

        ImGui::Separator();
        ImGui::Text( "Controls:" );

        render_control_item( item.controls );
    }
    ImGui::PopID();
}

void hdr_model::render_hdr_config_window( ux_window & window, std::string & error_message )
{
    const auto window_name = "HDR Configuration";
    if (_window_open)
    {
        ImGui::OpenPopup(window_name);
        _window_open = false;
    }

    ImGui::SetNextWindowSize({ 680, 600 }, ImGuiCond_Appearing);

    auto flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;

    ImGui::PushStyleColor(ImGuiCol_PopupBg, sensor_bg);
    ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, light_grey);
    ImGui::PushStyleColor(ImGuiCol_Text, light_grey);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 1);
    ImGui::PushStyleColor(ImGuiCol_Button, button_color);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, button_color + 0.1f);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, button_color + 0.1f);

    if (ImGui::BeginPopupModal(window_name, nullptr, flags))
    {
        float button_area = ImGui::GetFrameHeightWithSpacing() * 1.5f;
        ImVec2 btnSize(120, 0);
        if (ImGui::BeginChild("HDR Container", ImVec2(0, -button_area), true))
        {
            if (!error_message.empty())
                ImGui::TextColored({ 1,0.3f,0.3f,1 }, "%s", error_message.c_str());

            ImGui::Text("Preset ID:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(150);
            char buf[32];
            strncpy(buf, _changed_config.id.c_str(), 31);
            buf[31] = '\0';
            if (ImGui::InputText("##id", buf, 32))
                _changed_config.id = buf;

            ImGui::Text("Global Iterations:");
            // tooltip
            if (ImGui::IsItemHovered())
                RsImGui::CustomTooltip("Number of times the specified preset will be run, 0 - infinite");

            ImGui::SameLine();
            int gi = _changed_config.iterations;
            ImGui::SetNextItemWidth(150);
            if (ImGui::InputInt("##gi", &gi))
                _changed_config.iterations = std::max(0, gi);

            ImGui::Spacing();
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.5f, 1.0f), "Preset Items");
            ImGui::SameLine();
            RsImGui::RsImButton(
                [&]()
                {
                    if (ImGui::Button("-", { 22, 22 }))
                    {
                        _changed_config.items.pop_back();
                    }
                },
                _changed_config.items.size() <= 2);

            ImGui::SameLine();
            RsImGui::RsImButton(
                [&]()
                {

                    if (ImGui::Button("+", { 22, 22 }))
                    {
                        hdr_preset::control_item control = hdr_preset::control_item((int)_gain_range.def, (int)_exp_range.def);
                        hdr_preset::preset_item item;
                        item.iterations = 1;
                        item.controls = control;

                        _changed_config.items.push_back(item);
                    }
                },
                _changed_config.items.size() >= 6);

            ImGui::Separator();

            for (size_t i = 0; i < _changed_config.items.size(); ++i)
                render_preset_item(_changed_config.items[i], int(i));

            ImGui::Spacing();
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.5f, 1.0f), "Load/Save Configuration");
            ImGui::Separator();
            if (ImGui::Button("Load defaults", btnSize))
            {
                _changed_config = _default_config;
            }
            ImGui::SameLine();

            if (ImGui::Button("Load from File", btnSize))
            {
                auto ret = file_dialog_open(open_file, "JavaScript Object Notation (JSON)\0*.json\0", nullptr, nullptr);
                if (ret)
                {
                    try
                    {
                        load_hdr_config_from_file(ret);
                    }
                    catch (const std::exception& e)
                    {
                        error_message = e.what();
                    }
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Save to File", btnSize))
            {
                auto ret = file_dialog_open(save_file, "JavaScript Object Notation (JSON)\0*.json\0", nullptr, nullptr);
                if (ret)
                {
                    try
                    {
                        save_hdr_config_to_file(ret);
                    }
                    catch (const std::exception& e)
                    {
                        error_message = e.what();
                    }
                }
            }
        }
        ImGui::EndChild();

        // center bottom buttons
        ImGui::SetCursorPosX( ( ImGui::GetContentRegionAvail().x - ( btnSize.x * 3 + 10 ) ) / 2 );
        ImGui::SetCursorPosY( ImGui::GetCursorPosY() + 10 );

        if (ImGui::Button("OK", btnSize))
        {
            apply_hdr_config();
            error_message.clear();
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        bool configs_same = _changed_config == _current_config;
        ImGui::PushStyleColor(ImGuiCol_Text, configs_same ? light_grey : light_blue);
        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, configs_same ? light_grey : light_blue);
        if (ImGui::Button("Apply", btnSize))
        {
            try {
                apply_hdr_config();
                error_message.clear();
            }
            catch (const std::exception& e) {
                error_message = e.what();
            }
        }
        ImGui::PopStyleColor(2);

        ImGui::SameLine();

        if (ImGui::Button("Cancel", btnSize))
        {
            close_window();
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    ImGui::PopStyleColor(6);
    ImGui::PopStyleVar(2);
}


void hdr_model::open_hdr_tool_window()
{
    _window_open = true;
}

void hdr_model::close_window()
{
    _window_open = false;
}

}
