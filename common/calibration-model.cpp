// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#include <rs-config.h>
#include "calibration-model.h"
#include "device-model.h"
#include "os.h"
#include "ux-window.h"
#include <realsense_imgui.h>

#include "../src/ds/d400/d400-private.h"
#include "../src/ds/d500/d500-private.h"


using namespace rs2;
namespace {
    constexpr float LEFT_ALIGN_LABELS = 10.0f;
    constexpr float FIRST_COLUMN_LOCATION = 200.0f;
    constexpr float SECOND_COLUMN_LOCATION = 400.0f;
    constexpr float INPUT_WIDTH = 120.0f;
}

calibration_model::calibration_model(rs2::device dev, std::shared_ptr<notifications_model> not_model)
    : dev(dev), _not_model(not_model)
{
    _accept = config_file::instance().get_or_default(configurations::calibration::enable_writing, false);
    if( dev.supports( RS2_CAMERA_INFO_PRODUCT_LINE ) ) {
        std::string product_line = dev.get_info( RS2_CAMERA_INFO_PRODUCT_LINE );
        d400_device = ( product_line == "D400" );
        d500_device = ( product_line == "D500" );
    }
}

bool calibration_model::supports()
{
    return dev.is<rs2::auto_calibrated_device>();
}

void calibration_model::draw_float(std::string name, float& x, const float& orig, bool& changed)
{
    if( std::abs( x - orig ) > 0.00001 )
        ImGui::PushStyleColor( ImGuiCol_FrameBg, regular_blue );
    else ImGui::PushStyleColor(ImGuiCol_FrameBg, black);
    if (ImGui::DragFloat(std::string( rsutils::string::from() << "##" << name).c_str(), &x, 0.001f))
    {
        changed = true;
    }
    ImGui::PopStyleColor();
}

void calibration_model::draw_int( std::string name, uint16_t & x, const uint16_t & orig, bool & changed )
{
    int temp = static_cast< int >( x );
    if( temp != static_cast< int >( orig ) )
        ImGui::PushStyleColor( ImGuiCol_FrameBg, regular_blue );
    else
        ImGui::PushStyleColor( ImGuiCol_FrameBg, black );
    std::string label = "##" + name;
    if( ImGui::DragInt( label.c_str(), &temp, 1, 0, 65535 ) )
    {
        uint16_t new_val = static_cast< uint16_t >( temp );
        if( new_val != x )
        {
            x = new_val;
            changed = true;
        }
    }

    ImGui::PopStyleColor();
}

void calibration_model::draw_read_only_int(std::string name, int x)
{
    ImGui::BeginDisabled();
    ImGui::DragInt(("##" + name).c_str(), &x);
    ImGui::EndDisabled();
}

void calibration_model::draw_read_only_float(std::string name, float x)
{
    ImGui::BeginDisabled();
    ImGui::DragFloat(("##" + name).c_str(), &x);
    ImGui::EndDisabled();
}

void calibration_model::draw_float4x4(std::string name, float3x3& field,
                                      const float3x3& original, bool& changed)
{
    ImGui::SetCursorPosX(LEFT_ALIGN_LABELS);
    ImGui::Text("%s:", name.c_str()); ImGui::SameLine();

    ImGui::PushItemWidth(INPUT_WIDTH);

    ImGui::SetCursorPosX(FIRST_COLUMN_LOCATION);
    draw_float(name + "_XX", field.x.x, original.x.x, changed);
    ImGui::SameLine();
    draw_float(name + "_XY", field.x.y, original.x.y, changed);
    ImGui::SameLine();
    draw_float(name + "_XZ", field.x.z, original.x.z, changed);

    ImGui::SetCursorPosX(FIRST_COLUMN_LOCATION);
    draw_float(name + "_YX", field.y.x, original.y.x, changed);
    ImGui::SameLine();
    draw_float(name + "_YY", field.y.y, original.y.y, changed);
    ImGui::SameLine();
    draw_float(name + "_YZ", field.y.z, original.y.z, changed);

    ImGui::SetCursorPosX(FIRST_COLUMN_LOCATION);
    draw_float(name + "_ZX", field.z.x, original.z.x, changed);
    ImGui::SameLine();
    draw_float(name + "_ZY", field.z.y, original.z.y, changed);
    ImGui::SameLine();
    draw_float(name + "_ZZ", field.z.z, original.z.z, changed);

    ImGui::PopItemWidth();

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
}

void calibration_model::draw_intrinsics(std::string name, mini_intrinsics& field,
                                       const mini_intrinsics& original, bool& changed)
{
    ImGui::SetCursorPosX(LEFT_ALIGN_LABELS);
    ImGui::Text("%s:", name.c_str()); ImGui::SameLine();

    ImGui::PushItemWidth(INPUT_WIDTH);

    ImGui::SetCursorPosX(FIRST_COLUMN_LOCATION);
    ImGui::Text("%s:", "ppx"); ImGui::SameLine();
    ImGui::SetCursorPosX(FIRST_COLUMN_LOCATION + 50);
    draw_float(name + "_ppx", field.ppx, original.ppx, changed);
    ImGui::SameLine();
    ImGui::SetCursorPosX(SECOND_COLUMN_LOCATION);
    ImGui::Text("%s:", "ppy"); ImGui::SameLine();
    ImGui::SetCursorPosX(SECOND_COLUMN_LOCATION + 50);
    draw_float(name + "_ppy", field.ppy, original.ppy, changed);

    ImGui::SetCursorPosX(FIRST_COLUMN_LOCATION);
    ImGui::Text("%s:", "fx"); ImGui::SameLine();
    ImGui::SetCursorPosX(FIRST_COLUMN_LOCATION + 50);
    draw_float(name + "_fx", field.fx, original.fx, changed);
    ImGui::SameLine();
    ImGui::SetCursorPosX(SECOND_COLUMN_LOCATION);
    ImGui::Text("%s:", "fy"); ImGui::SameLine();
    ImGui::SetCursorPosX(SECOND_COLUMN_LOCATION + 50);
    draw_float(name + "_fy", field.fy, original.fy, changed);

    ImGui::SetCursorPosX(FIRST_COLUMN_LOCATION);
    ImGui::Text("%s:", "width"); ImGui::SameLine();
    ImGui::SetCursorPosX(FIRST_COLUMN_LOCATION + 50);
    draw_read_only_int(name + "width", field.image_width );
    ImGui::SameLine();
    ImGui::SetCursorPosX(SECOND_COLUMN_LOCATION);
    ImGui::Text("%s:", "height"); ImGui::SameLine();
    ImGui::SetCursorPosX(SECOND_COLUMN_LOCATION + 50);
    draw_read_only_int(name + "height", field.image_height );

    ImGui::PopItemWidth();

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
}

void calibration_model::draw_distortion(std::string name, librealsense::ds::d500_calibration_distortion& distortion_model, float(&distortion_coeffs)[13], const float(&original_coeffs)[13], bool& changed)
{
    std::string distortion_str = "Unknown";

    switch (distortion_model)
    {
    case librealsense::ds::d500_calibration_distortion::none:
        distortion_str = "None";
        break;
    case librealsense::ds::d500_calibration_distortion::brown:
        distortion_str = "Brown";
        break;
    case librealsense::ds::d500_calibration_distortion::brown_and_fisheye:
        distortion_str = "Brown and Fisheye";
        break;
    }

    ImGui::PushItemWidth(INPUT_WIDTH);

    ImGui::SetCursorPosX(LEFT_ALIGN_LABELS);
    ImGui::Text((name + " Model:").c_str()); ImGui::SameLine();
    ImGui::SetCursorPosX(FIRST_COLUMN_LOCATION);

    // Display distortion model in a read-only InputText (disabled) to align with editable fields
    ImGui::BeginDisabled();
    char distortion_buf[64]{};
    strncpy(distortion_buf, distortion_str.c_str(), sizeof(distortion_buf));
    ImGui::InputText(("##" + name).c_str(), distortion_buf, sizeof(distortion_buf));
    ImGui::EndDisabled();

    ImGui::SetCursorPosX(LEFT_ALIGN_LABELS);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
    ImGui::Text((name + " Params:").c_str()); ImGui::SameLine();

    ImGui::PopItemWidth();

    ImGui::SetCursorPosX(FIRST_COLUMN_LOCATION);
    ImGui::PushItemWidth(80);

    const int items_per_column = 5;
    const float column_width = 20;

    for (int col = 0; col < 3; col++)
    {
        ImGui::BeginGroup();
        int start_idx = col * items_per_column;
        int end_idx = std::min(start_idx + items_per_column, 13);

        for (int i = start_idx; i < end_idx; i++)
        {
            ImGui::Text("[%d]:", i);
            ImGui::SameLine();
            draw_float(name + "_" + std::to_string(i), distortion_coeffs[i], original_coeffs[i], changed);
        }
        ImGui::EndGroup();

        if (col < 2) ImGui::SameLine(0, column_width);
    }
        
    ImGui::PopItemWidth();
}

void calibration_model::update(ux_window& window, std::string& error_message)
{
    if (to_open)
    {
        try
        {
            _calibration = dev.as<rs2::auto_calibrated_device>().get_calibration_table();
            _original = _calibration;
            ImGui::OpenPopup("Calibration Window");
        }
        catch(std::exception e)
        {
            error_message = e.what();
        }
        to_open = false;
    }
    if (d400_device)
    {
        d400_update(window, error_message);
    }
    if (d500_device)
    {
        d500_update(window, error_message);
    }
}

void calibration_model::d400_update(ux_window& window, std::string& error_message)
{
    auto table = (librealsense::ds::d400_coefficients_table*)_calibration.data();
    auto orig_table = (librealsense::ds::d400_coefficients_table*)_original.data();
    bool changed = false;

    const float w = 620;
    const float h = 500;
    const float x0 = std::max(window.width() - w, 0.f) / 2;
    const float y0 = std::max(window.height() - h, 0.f) / 2;
    ImGui::SetNextWindowPos({ x0, y0 });
    ImGui::SetNextWindowSize({ w, h });

    auto flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;

    ImGui::PushStyleColor(ImGuiCol_PopupBg, sensor_bg);
    ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, white);
    ImGui::PushStyleColor(ImGuiCol_Text, light_grey);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 1);

    if (ImGui::BeginPopupModal("Calibration Window", nullptr, flags))
    {
        if (error_message != "") ImGui::CloseCurrentPopup();

        std::string title_message = "CAMERA CALIBRATION";
        auto title_size = ImGui::CalcTextSize(title_message.c_str());
        ImGui::SetCursorPosX(w / 2 - title_size.x / 2);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
        ImGui::PushFont(window.get_large_font());
        ImGui::PushStyleColor(ImGuiCol_Text, white);
        ImGui::Text("%s", title_message.c_str());
        ImGui::PopStyleColor();
        ImGui::PopFont();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);

        ImGui::SetCursorPosX(w / 2 - 260 / 2);
        if (ImGui::Button(u8"\uF07C Load...", ImVec2(70, 30)))
        {
            try
            {
                if (auto fn = file_dialog_open(file_dialog_mode::open_file, "Calibration JSON\0*.json\0", nullptr, nullptr))
                {
                    config_file cf(fn);
                    table->baseline = cf.get("baseline");

                    auto load_float3x3 = [&](std::string name, librealsense::float3x3& m){
                        m.x.x = cf.get(std::string( rsutils::string::from() << name << ".x.x").c_str());
                        m.x.y = cf.get(std::string( rsutils::string::from() << name << ".x.y").c_str());
                        m.x.z = cf.get(std::string( rsutils::string::from() << name << ".x.z").c_str());

                        m.y.x = cf.get(std::string( rsutils::string::from() << name << ".y.x").c_str());
                        m.y.y = cf.get(std::string( rsutils::string::from() << name << ".y.y").c_str());
                        m.y.z = cf.get(std::string( rsutils::string::from() << name << ".y.z").c_str());

                        m.z.x = cf.get(std::string( rsutils::string::from() << name << ".z.x").c_str());
                        m.z.y = cf.get(std::string( rsutils::string::from() << name << ".z.y").c_str());
                        m.z.z = cf.get(std::string( rsutils::string::from() << name << ".z.z").c_str());
                    };

                    load_float3x3("intrinsic_left", table->intrinsic_left);
                    load_float3x3("intrinsic_right", table->intrinsic_right);
                    load_float3x3("world2left_rot", table->world2left_rot);
                    load_float3x3("world2right_rot", table->world2right_rot);

                    for (int i = 0; i < librealsense::ds::max_ds_rect_resolutions; i++)
                    {
                        table->rect_params[i].x = cf.get(std::string( rsutils::string::from() << "rectified." << i << ".fx").c_str());
                        table->rect_params[i].y = cf.get(std::string( rsutils::string::from() << "rectified." << i << ".fy").c_str());

                        table->rect_params[i].z = cf.get(std::string( rsutils::string::from() << "rectified." << i << ".ppx").c_str());
                        table->rect_params[i].w = cf.get(std::string( rsutils::string::from() << "rectified." << i << ".ppy").c_str());
                    }
                }

                changed = true;
            }
            catch (const std::exception& ex)
            {
                error_message = ex.what();
                ImGui::CloseCurrentPopup();
            }
        }
        if (ImGui::IsItemHovered())
        {
            window.link_hovered();
            RsImGui::CustomTooltip("%s", "Load calibration from file");
        }
        ImGui::SameLine();
        if (ImGui::Button(u8"\uF0C7 Save As...", ImVec2(100, 30)))
        {
            try
            {
                if (auto fn = file_dialog_open(file_dialog_mode::save_file, "Calibration JSON\0*.json\0", nullptr, nullptr))
                {
                    config_file cf(fn);
                    cf.set("baseline", table->baseline);

                    auto save_float3x3 = [&](std::string name, librealsense::float3x3& m){
                        cf.set(std::string( rsutils::string::from() << name << ".x.x").c_str(), m.x.x);
                        cf.set(std::string( rsutils::string::from() << name << ".x.y").c_str(), m.x.y);
                        cf.set(std::string( rsutils::string::from() << name << ".x.z").c_str(), m.x.z);

                        cf.set(std::string( rsutils::string::from() << name << ".y.x").c_str(), m.y.x);
                        cf.set(std::string( rsutils::string::from() << name << ".y.y").c_str(), m.y.y);
                        cf.set(std::string( rsutils::string::from() << name << ".y.z").c_str(), m.y.z);

                        cf.set(std::string( rsutils::string::from() << name << ".z.x").c_str(), m.z.x);
                        cf.set(std::string( rsutils::string::from() << name << ".z.y").c_str(), m.z.y);
                        cf.set(std::string( rsutils::string::from() << name << ".z.z").c_str(), m.z.z);
                    };

                    save_float3x3("intrinsic_left", table->intrinsic_left);
                    save_float3x3("intrinsic_right", table->intrinsic_right);
                    save_float3x3("world2left_rot", table->world2left_rot);
                    save_float3x3("world2right_rot", table->world2right_rot);

                    for (int i = 0; i < librealsense::ds::max_ds_rect_resolutions; i++)
                    {
                        auto xy = librealsense::ds::resolutions_list[(librealsense::ds::ds_rect_resolutions)i];
                        int w = xy.x; int h = xy.y;

                        cf.set(std::string( rsutils::string::from() << "rectified." << i << ".width").c_str(), w);
                        cf.set(std::string( rsutils::string::from() << "rectified." << i << ".height").c_str(), h);

                        cf.set(std::string( rsutils::string::from() << "rectified." << i << ".fx").c_str(), table->rect_params[i].x);
                        cf.set(std::string( rsutils::string::from() << "rectified." << i << ".fy").c_str(), table->rect_params[i].y);

                        cf.set(std::string( rsutils::string::from() << "rectified." << i << ".ppx").c_str(), table->rect_params[i].z);
                        cf.set(std::string( rsutils::string::from() << "rectified." << i << ".ppy").c_str(), table->rect_params[i].w);
                    }
                }
            }
            catch (const std::exception& ex)
            {
                error_message = ex.what();
                ImGui::CloseCurrentPopup();
            }
        }
        if (ImGui::IsItemHovered())
        {
            window.link_hovered();
            RsImGui::CustomTooltip("%s", "Save calibration image to file");
        }
        ImGui::SameLine();
        if (_accept)
        {
            if (ImGui::Button(u8"\uF275 Restore Factory", ImVec2(120, 30)))
            {
                try
                {
                    dev.as<rs2::auto_calibrated_device>().reset_to_factory_calibration();
                    _calibration = dev.as<rs2::auto_calibrated_device>().get_calibration_table();
                    _original = _calibration;
                    table = reinterpret_cast< librealsense::ds::d400_coefficients_table * >( _calibration.data() );
                    orig_table = reinterpret_cast< librealsense::ds::d400_coefficients_table * >( _original.data() );
                    changed = true;

                    if (auto nm = _not_model.lock())
                    {
                        nm->add_notification({ rsutils::string::from() << "Depth Calibration is reset to Factory Settings",
                            RS2_LOG_SEVERITY_INFO, RS2_NOTIFICATION_CATEGORY_HARDWARE_EVENT });
                    }
                }
                catch(const std::exception& ex)
                {
                    error_message = ex.what();
                    ImGui::CloseCurrentPopup();
                }
            }
            if (ImGui::IsItemHovered())
            {
                window.link_hovered();
                RsImGui::CustomTooltip("%s", "Restore calibration in flash to factory settings");
            }
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_Text, grey);
            ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, grey);

            ImGui::Button(u8"\uF275 Restore Factory", ImVec2(120, 30));
            if (ImGui::IsItemHovered())
            {
                RsImGui::CustomTooltip("%s", "Write selected calibration table to the device. For advanced users");
            }

            ImGui::PopStyleColor(2);
        }

        ImGui::PushStyleColor(ImGuiCol_ChildBg, dark_sensor_bg);

        ImGui::BeginChild("##CalibData",ImVec2(w - 15, h - 110), true);

        ImGui::SetCursorPosX(LEFT_ALIGN_LABELS);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);

        ImGui::Text("Stereo Baseline (mm):"); ImGui::SameLine();
        ImGui::SetCursorPosX(FIRST_COLUMN_LOCATION);

        ImGui::PushItemWidth(INPUT_WIDTH);
        draw_float("Baseline", table->baseline, orig_table->baseline, changed);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
        ImGui::PopItemWidth();

        draw_float4x4("Left Intrinsics", table->intrinsic_left, orig_table->intrinsic_left, changed);
        draw_float4x4("Right Intrinsics", table->intrinsic_right, orig_table->intrinsic_right, changed);
        draw_float4x4("World to Left Rotation", table->world2left_rot, orig_table->world2left_rot, changed);
        draw_float4x4("World to Right Rotation", table->world2right_rot, orig_table->world2right_rot, changed);

        ImGui::SetCursorPosX(LEFT_ALIGN_LABELS);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);

        ImGui::Text("Rectified Resolution:"); ImGui::SameLine();
        ImGui::SetCursorPosX(FIRST_COLUMN_LOCATION);

        std::vector<std::string> resolution_names;
        std::vector<const char*> resolution_names_char;
        std::vector<int> resolution_offset;
        for (int i = 0; i < librealsense::ds::max_ds_rect_resolutions; i++)
        {
            auto xy = librealsense::ds::resolutions_list[(librealsense::ds::ds_rect_resolutions)i];
            int w = xy.x; int h = xy.y;
            if (w != 0) {
                resolution_offset.push_back(i);
                std::string name = rsutils::string::from() << w << " x " << h;
                resolution_names.push_back(name);
            }
        }
        for (size_t i = 0; i < resolution_offset.size(); i++)
        {
            resolution_names_char.push_back(resolution_names[i].c_str());
        }

        ImGui::PushItemWidth(INPUT_WIDTH);
        ImGui::Combo("##RectifiedResolutions", &selected_resolution, resolution_names_char.data(), int(resolution_names_char.size()));

        ImGui::SetCursorPosX(LEFT_ALIGN_LABELS);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);

        ImGui::Text("Focal Length:"); ImGui::SameLine();
        ImGui::SetCursorPosX(FIRST_COLUMN_LOCATION);

        draw_float("FocalX", table->rect_params[selected_resolution].x, orig_table->rect_params[selected_resolution].x, changed);
        ImGui::SameLine();
        draw_float("FocalY", table->rect_params[selected_resolution].y, orig_table->rect_params[selected_resolution].y, changed);

        ImGui::SetCursorPosX(LEFT_ALIGN_LABELS);
        ImGui::Text("Principal Point:"); ImGui::SameLine();
        ImGui::SetCursorPosX(FIRST_COLUMN_LOCATION);

        draw_float("PPX", table->rect_params[selected_resolution].z, orig_table->rect_params[selected_resolution].z, changed);
        ImGui::SameLine();
        draw_float("PPY", table->rect_params[selected_resolution].w, orig_table->rect_params[selected_resolution].w, changed);

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);

        ImGui::PopItemWidth();

        if (ImGui::IsWindowHovered()) window.set_hovered_over_input();

        ImGui::EndChild();
        ImGui::PopStyleColor();

        ImGui::SetCursorScreenPos({ (float)(x0 + 10), (float)(y0 + h - 30) });
        if (ImGui::Checkbox("I know what I'm doing", &_accept))
        {
            config_file::instance().set(configurations::calibration::enable_writing, _accept);
        }
        if (ImGui::IsItemHovered())
        {
            RsImGui::CustomTooltip("%s", "Changing calibration will affect depth quality. Changes are persistent.\nThere is an option to get back to factory calibration, but it may be worse than current calibration\nBefore writing to flash, we strongly recommend to make a file backup");
        }

        ImGui::SetCursorScreenPos({ (float)(x0 + w - 230), (float)(y0 + h - 30) });

        if (ImGui::Button("Cancel", ImVec2(100, 25)))
        {
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::IsItemHovered())
        {
            window.link_hovered();
            RsImGui::CustomTooltip("%s", "Close without saving any changes");
        }
        ImGui::SameLine();

        if (_accept)
        {
            if (ImGui::Button(u8"\uF2DB  Write Table", ImVec2(120, 25)))
            {
                try
                {
                    auto actual_data = _calibration.data() + sizeof(librealsense::ds::table_header);
                    auto actual_data_size = _calibration.size() - sizeof(librealsense::ds::table_header);
                    auto crc = rsutils::number::calc_crc32( actual_data, actual_data_size );
                    table->header.crc32 = crc;
                    dev.as<rs2::auto_calibrated_device>().set_calibration_table(_calibration);
                    dev.as<rs2::auto_calibrated_device>().write_calibration();
                    _original = _calibration;
                    orig_table = reinterpret_cast< librealsense::ds::d400_coefficients_table * >( _original.data() );
                    ImGui::CloseCurrentPopup();
                }
                catch (const std::exception& ex)
                {
                    error_message = ex.what();
                    ImGui::CloseCurrentPopup();
                }
            }
            if (ImGui::IsItemHovered())
            {
                window.link_hovered();
                RsImGui::CustomTooltip("%s", "Write selected calibration table to the device");
            }
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_Text, grey);
            ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, grey);

            ImGui::Button(u8"\uF2DB  Write Table", ImVec2(120, 25));
            if (ImGui::IsItemHovered())
            {
                RsImGui::CustomTooltip("%s", "Write selected calibration table to the device. For advanced users");
            }

            ImGui::PopStyleColor(2);
        }
        if (changed)
        {
            try
            {
                dev.as<rs2::auto_calibrated_device>().set_calibration_table(_calibration);
            }
            catch (const std::exception&)
            {
                try
                {
                    auto streams = dev.query_sensors()[0].get_active_streams();
                    dev.query_sensors()[0].close();
                    dev.query_sensors()[0].open(streams);
                    dev.as<rs2::auto_calibrated_device>().set_calibration_table(_calibration);
                }
                catch (const std::exception& ex)
                {
                    error_message = ex.what();
                    ImGui::CloseCurrentPopup();
                }
            }
        }

        if (ImGui::IsWindowHovered()) window.set_hovered_over_input();

        ImGui::EndPopup();
    }
    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(2);
}

void calibration_model::d500_update( ux_window & window, std::string & error_message )
{
    auto table = (librealsense::ds::d500_coefficients_table*)_calibration.data();
    auto orig_table = (librealsense::ds::d500_coefficients_table*)_original.data();
    bool changed = false;

    const float w = 620;
    const float h = 500;
    const float x0 = std::max(window.width() - w, 0.f) / 2;
    const float y0 = std::max(window.height() - h, 0.f) / 2;
    ImGui::SetNextWindowPos({ x0, y0 });
    ImGui::SetNextWindowSize({ w, h });

    auto flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;

    ImGui::PushStyleColor(ImGuiCol_PopupBg, sensor_bg);
    ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, white);
    ImGui::PushStyleColor(ImGuiCol_Text, light_grey);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 1);

    if (ImGui::BeginPopupModal("Calibration Window", nullptr, flags))
    {
        if (error_message != "") ImGui::CloseCurrentPopup();

        std::string title_message = "CAMERA CALIBRATION";
        auto title_size = ImGui::CalcTextSize(title_message.c_str());
        ImGui::SetCursorPosX(w / 2 - title_size.x / 2);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
        ImGui::PushFont(window.get_large_font());
        ImGui::PushStyleColor(ImGuiCol_Text, white);
        ImGui::Text("%s", title_message.c_str());
        ImGui::PopStyleColor();
        ImGui::PopFont();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);

        ImGui::SetCursorPosX(w / 2 - 260 / 2);
        if (ImGui::Button(u8"\uF07C Load...", ImVec2(70, 30)))
        {
            try
            {
                if (auto fn = file_dialog_open(file_dialog_mode::open_file, "Calibration JSON\0*.json\0", nullptr, nullptr))
                {
                    config_file cf(fn);

                    auto load_float3x3 = [&](std::string name, librealsense::float3x3& m){
                        m.x.x = cf.get(std::string( rsutils::string::from() << name << ".x.x").c_str());
                        m.x.y = cf.get(std::string( rsutils::string::from() << name << ".x.y").c_str());
                        m.x.z = cf.get(std::string( rsutils::string::from() << name << ".x.z").c_str());

                        m.y.x = cf.get(std::string( rsutils::string::from() << name << ".y.x").c_str());
                        m.y.y = cf.get(std::string( rsutils::string::from() << name << ".y.y").c_str());
                        m.y.z = cf.get(std::string( rsutils::string::from() << name << ".y.z").c_str());

                        m.z.x = cf.get(std::string( rsutils::string::from() << name << ".z.x").c_str());
                        m.z.y = cf.get(std::string( rsutils::string::from() << name << ".z.y").c_str());
                        m.z.z = cf.get(std::string( rsutils::string::from() << name << ".z.z").c_str());
                    };

                    auto load_intrinsics = [&]( std::string name, mini_intrinsics & m ) {
                        m.ppx = cf.get( std::string( rsutils::string::from() << name << ".ppx" ).c_str() );
                        m.ppy = cf.get( std::string( rsutils::string::from() << name << ".ppy" ).c_str() );

                        m.fx = cf.get( std::string( rsutils::string::from() << name << ".fx" ).c_str() );
                        m.fy = cf.get( std::string( rsutils::string::from() << name << ".fy" ).c_str() );
                    };

                    auto load_distortion_params = [&](std::string name, float(&distortion_coeffs)[13]) {
                        for (int i = 0; i < 13; ++i)
                        {
                            distortion_coeffs[i] = cf.get(std::string(rsutils::string::from() << name << "." << i ).c_str());
                        }
                    };

                    load_intrinsics( "left_coefficients_table", table->left_coefficients_table.base_instrinsics );
                    load_intrinsics( "right_coefficients_table", table->right_coefficients_table.base_instrinsics );
                    load_float3x3( "left_rotation_matrix", table->left_coefficients_table.rotation_matrix );
                    load_float3x3( "right_rotation_matrix", table->right_coefficients_table.rotation_matrix );
                    load_distortion_params("left_distortion_coeffs", table->left_coefficients_table.distortion_coeffs);
                    load_distortion_params("right_distortion_coeffs", table->right_coefficients_table.distortion_coeffs);

                }

                changed = true;
            }
            catch (const std::exception& ex)
            {
                error_message = ex.what();
                ImGui::CloseCurrentPopup();
            }
        }
        if (ImGui::IsItemHovered())
        {
            window.link_hovered();
            RsImGui::CustomTooltip("%s", "Load calibration from file");
        }
        ImGui::SameLine();
        if (ImGui::Button(u8"\uF0C7 Save As...", ImVec2(100, 30)))
        {
            try
            {
                if (auto fn = file_dialog_open(file_dialog_mode::save_file, "Calibration JSON\0*.json\0", nullptr, nullptr))
                {
                    config_file cf(fn);

                    auto save_float3x3 = [&](std::string name, librealsense::float3x3& m){
                        cf.set(std::string( rsutils::string::from() << name << ".x.x").c_str(), m.x.x);
                        cf.set(std::string( rsutils::string::from() << name << ".x.y").c_str(), m.x.y);
                        cf.set(std::string( rsutils::string::from() << name << ".x.z").c_str(), m.x.z);

                        cf.set(std::string( rsutils::string::from() << name << ".y.x").c_str(), m.y.x);
                        cf.set(std::string( rsutils::string::from() << name << ".y.y").c_str(), m.y.y);
                        cf.set(std::string( rsutils::string::from() << name << ".y.z").c_str(), m.y.z);

                        cf.set(std::string( rsutils::string::from() << name << ".z.x").c_str(), m.z.x);
                        cf.set(std::string( rsutils::string::from() << name << ".z.y").c_str(), m.z.y);
                        cf.set(std::string( rsutils::string::from() << name << ".z.z").c_str(), m.z.z);
                    };

                    auto save_intrinsics = [&]( std::string name, mini_intrinsics & m ) {
                        cf.set( std::string( rsutils::string::from() << name << ".ppx" ).c_str(), m.ppx );
                        cf.set( std::string( rsutils::string::from() << name << ".ppy" ).c_str(), m.ppy );

                        cf.set( std::string( rsutils::string::from() << name << ".fx" ).c_str(), m.fx );
                        cf.set( std::string( rsutils::string::from() << name << ".fy" ).c_str(), m.fy );
                    };

                    auto save_distortion_params = [&](std::string name, float(&distortion_coeffs)[13]) {
                        for (int i = 0; i < 13; ++i)
                        {
                            cf.set(std::string(rsutils::string::from() << name << "." << i).c_str(), distortion_coeffs[i]);
                        }
                    };

                    save_intrinsics( "left_coefficients_table", table->left_coefficients_table.base_instrinsics );
                    save_intrinsics( "right_coefficients_table", table->right_coefficients_table.base_instrinsics );
                    save_float3x3( "left_rotation_matrix", table->left_coefficients_table.rotation_matrix );
                    save_float3x3( "right_rotation_matrix", table->right_coefficients_table.rotation_matrix );
                    save_distortion_params("left_distortion_coeffs", table->left_coefficients_table.distortion_coeffs);
                    save_distortion_params("right_distortion_coeffs", table->right_coefficients_table.distortion_coeffs);
                }
            }
            catch (const std::exception& ex)
            {
                error_message = ex.what();
                ImGui::CloseCurrentPopup();
            }
        }
        if (ImGui::IsItemHovered())
        {
            window.link_hovered();
            RsImGui::CustomTooltip("%s", "Save calibration image to file");
        }
        ImGui::SameLine();
        if (_accept)
        {
            if (ImGui::Button(u8"\uF275 Restore Factory", ImVec2(120, 30)))
            {
                try
                {
                    dev.as<rs2::auto_calibrated_device>().reset_to_factory_calibration();
                    _calibration = dev.as<rs2::auto_calibrated_device>().get_calibration_table();
                    _original = _calibration;
                    table = reinterpret_cast< librealsense::ds::d500_coefficients_table * >( _calibration.data() );
                    orig_table = reinterpret_cast< librealsense::ds::d500_coefficients_table * >( _original.data() );
                    changed = true;

                    if (auto nm = _not_model.lock())
                    {
                        nm->add_notification({ rsutils::string::from() << "Depth Calibration is reset to Factory Settings",
                                              RS2_LOG_SEVERITY_INFO, RS2_NOTIFICATION_CATEGORY_HARDWARE_EVENT });
                    }
                }
                catch(const std::exception& ex)
                {
                    error_message = ex.what();
                    ImGui::CloseCurrentPopup();
                }
            }
            if (ImGui::IsItemHovered())
            {
                window.link_hovered();
                RsImGui::CustomTooltip("%s", "Restore calibration in flash to factory settings");
            }
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_Text, grey);
            ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, grey);

            ImGui::Button(u8"\uF275 Restore Factory", ImVec2(120, 30));
            if (ImGui::IsItemHovered())
            {
                RsImGui::CustomTooltip("%s", "Write selected calibration table to the device. For advanced users");
            }

            ImGui::PopStyleColor(2);
        }

        ImGui::PushStyleColor(ImGuiCol_ChildBg, dark_sensor_bg);

        ImGui::BeginChild("##CalibData",ImVec2(w - 15, h - 110), true);

        ImGui::PushItemWidth(INPUT_WIDTH);

        //Baseline
        ImGui::SetCursorPosX(LEFT_ALIGN_LABELS);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);

        ImGui::Text("Baseline (mm):"); ImGui::SameLine();
        ImGui::SetCursorPosX(FIRST_COLUMN_LOCATION);
        draw_read_only_float("Baseline", table->baseline);

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);

        //Left Camera
        draw_intrinsics( "Left Intrinsics", table->left_coefficients_table.base_instrinsics, orig_table->left_coefficients_table.base_instrinsics, changed );
        draw_distortion("Left Distortion", table->left_coefficients_table.distortion_model, table->left_coefficients_table.distortion_coeffs, orig_table->left_coefficients_table.distortion_coeffs, changed);
        draw_float4x4( "Left Rotation Matrix", table->left_coefficients_table.rotation_matrix, orig_table->left_coefficients_table.rotation_matrix, changed );
        
        //Right Camera
        draw_intrinsics("Right Intrinsics", table->right_coefficients_table.base_instrinsics, orig_table->right_coefficients_table.base_instrinsics, changed);
        draw_distortion("Right Distortion", table->right_coefficients_table.distortion_model, table->right_coefficients_table.distortion_coeffs, orig_table->right_coefficients_table.distortion_coeffs, changed);
        draw_float4x4( "Right Rotation Matrix", table->right_coefficients_table.rotation_matrix, orig_table->right_coefficients_table.rotation_matrix, changed );
        
        //Rectified image
        ImGui::BeginDisabled();
        draw_intrinsics("Rectified Intrinsics", table->rectified_intrinsics, orig_table->rectified_intrinsics, changed);
        ImGui::EndDisabled();

        ImGui::SetCursorPosX(LEFT_ALIGN_LABELS);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
        ImGui::Text("Realignment Essential:"); ImGui::SameLine();
        ImGui::SetCursorPosX(FIRST_COLUMN_LOCATION);
        draw_read_only_int("Realignment Essential", table->realignment_essential);

        ImGui::SetCursorPosX(LEFT_ALIGN_LABELS);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
        ImGui::Text("Vertical Shift:"); ImGui::SameLine();
        ImGui::SetCursorPosX(FIRST_COLUMN_LOCATION);
        draw_read_only_int("Vertical Shift", table->vertical_shift);
       
        ImGui::PopItemWidth();
        ImGui::SetCursorPosX(LEFT_ALIGN_LABELS);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);

        ImGui::SetCursorPosX(FIRST_COLUMN_LOCATION);

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);

        if (ImGui::IsWindowHovered()) window.set_hovered_over_input();

        ImGui::EndChild();
        ImGui::PopStyleColor();

        ImGui::SetCursorScreenPos({ (float)(x0 + 10), (float)(y0 + h - 30) });
        if (ImGui::Checkbox("I know what I'm doing", &_accept))
        {
            config_file::instance().set(configurations::calibration::enable_writing, _accept);
        }
        if (ImGui::IsItemHovered())
        {
            RsImGui::CustomTooltip("%s", "Changing calibration will affect depth quality. Changes are persistent.\nThere is an option to get back to factory calibration, but it may be worse than current calibration\nBefore writing to flash, we strongly recommend to make a file backup");
        }

        ImGui::SetCursorScreenPos({ (float)(x0 + w - 230), (float)(y0 + h - 30) });

        if (ImGui::Button("Cancel", ImVec2(100, 25)))
        {
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::IsItemHovered())
        {
            window.link_hovered();
            RsImGui::CustomTooltip("%s", "Close without saving any changes");
        }
        ImGui::SameLine();

        if (_accept)
        {
            if (ImGui::Button(u8"\uF2DB  Write Table", ImVec2(120, 25)))
            {
                try
                {
                    auto actual_data = _calibration.data() + sizeof(librealsense::ds::table_header);
                    auto actual_data_size = _calibration.size() - sizeof(librealsense::ds::table_header);
                    auto crc = rsutils::number::calc_crc32( actual_data, actual_data_size );
                    table->header.crc32 = crc;
                    dev.as<rs2::auto_calibrated_device>().set_calibration_table(_calibration);
                    dev.as<rs2::auto_calibrated_device>().write_calibration();
                    _original = _calibration;
                    orig_table = reinterpret_cast< librealsense::ds::d500_coefficients_table * >( _original.data() );
                    ImGui::CloseCurrentPopup();
                }
                catch (const std::exception& ex)
                {
                    error_message = ex.what();
                    ImGui::CloseCurrentPopup();
                }
            }
            if (ImGui::IsItemHovered())
            {
                window.link_hovered();
                RsImGui::CustomTooltip("%s", "Write selected calibration table to the device");
            }
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_Text, grey);
            ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, grey);

            ImGui::Button(u8"\uF2DB  Write Table", ImVec2(120, 25));
            if (ImGui::IsItemHovered())
            {
                RsImGui::CustomTooltip("%s", "Write selected calibration table to the device. For advanced users");
            }

            ImGui::PopStyleColor(2);
        }
        if (changed)
        {
            try
            {
                dev.as<rs2::auto_calibrated_device>().set_calibration_table(_calibration);
            }
            catch (const std::exception&)
            {
                try
                {
                    auto streams = dev.query_sensors()[0].get_active_streams();
                    dev.query_sensors()[0].close();
                    dev.query_sensors()[0].open(streams);
                    dev.as<rs2::auto_calibrated_device>().set_calibration_table(_calibration);
                }
                catch (const std::exception& ex)
                {
                    error_message = ex.what();
                    ImGui::CloseCurrentPopup();
                }
            }
        }

        if (ImGui::IsWindowHovered()) window.set_hovered_over_input();

        ImGui::EndPopup();
    }
    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(2);
}
