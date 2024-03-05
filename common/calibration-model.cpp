// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#include <rs-config.h>
#include "calibration-model.h"
#include "device-model.h"
#include "os.h"
#include "ux-window.h"

#include "../src/ds/d400/d400-private.h"


using namespace rs2;

calibration_model::calibration_model(rs2::device dev, std::shared_ptr<notifications_model> not_model)
    : dev(dev), _not_model(not_model)
{
    _accept = config_file::instance().get_or_default(configurations::calibration::enable_writing, false);
}

bool calibration_model::supports()
{
    bool is_d400 = dev.supports(RS2_CAMERA_INFO_PRODUCT_LINE) ?
        std::string(dev.get_info(RS2_CAMERA_INFO_PRODUCT_LINE)) == "D400" : false;

    return dev.is<rs2::auto_calibrated_device>() && is_d400;
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

void calibration_model::draw_float4x4(std::string name, float3x3& feild,
                                      const float3x3& original, bool& changed)
{
    ImGui::SetCursorPosX(10);
    ImGui::Text("%s:", name.c_str()); ImGui::SameLine();
    ImGui::SetCursorPosX(200);

    ImGui::PushItemWidth(120);
    ImGui::SetCursorPosX(200);
    draw_float(name + "_XX", feild.x.x, original.x.x, changed);
    ImGui::SameLine();
    draw_float(name + "_XY", feild.x.y, original.x.y, changed);
    ImGui::SameLine();
    draw_float(name + "_XZ", feild.x.z, original.x.z, changed);

    ImGui::SetCursorPosX(200);
    draw_float(name + "_YX", feild.y.x, original.y.x, changed);
    ImGui::SameLine();
    draw_float(name + "_YY", feild.y.y, original.y.y, changed);
    ImGui::SameLine();
    draw_float(name + "_YZ", feild.y.z, original.y.z, changed);

    ImGui::SetCursorPosX(200);
    draw_float(name + "_ZX", feild.z.x, original.z.x, changed);
    ImGui::SameLine();
    draw_float(name + "_ZY", feild.z.y, original.z.y, changed);
    ImGui::SameLine();
    draw_float(name + "_ZZ", feild.z.z, original.z.z, changed);

    ImGui::PopItemWidth();

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
}

void calibration_model::update(ux_window& window, std::string& error_message)
{
    const auto window_name = "Calibration Window";

    if (to_open)
    {
        try
        {
            _calibration = dev.as<rs2::auto_calibrated_device>().get_calibration_table();
            _original = _calibration;
            ImGui::OpenPopup(window_name);
        }
        catch(std::exception e)
        {
            error_message = e.what();
        }
        to_open = false;
    }

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

    if (ImGui::BeginPopupModal(window_name, nullptr, flags))
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

                    auto load_float3x4 = [&](std::string name, librealsense::float3x3& m){
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

                    load_float3x4("intrinsic_left", table->intrinsic_left);
                    load_float3x4("intrinsic_right", table->intrinsic_right);
                    load_float3x4("world2left_rot", table->world2left_rot);
                    load_float3x4("world2right_rot", table->world2right_rot);

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
            ImGui::SetTooltip("%s", "Load calibration from file");
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

                    auto save_float3x4 = [&](std::string name, librealsense::float3x3& m){
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

                    save_float3x4("intrinsic_left", table->intrinsic_left);
                    save_float3x4("intrinsic_right", table->intrinsic_right);
                    save_float3x4("world2left_rot", table->world2left_rot);
                    save_float3x4("world2right_rot", table->world2right_rot);

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
            ImGui::SetTooltip("%s", "Save calibration image to file");
        }
        ImGui::SameLine();
        if (_accept)
        {
            if (ImGui::Button(u8"\uF275 Restore Factory", ImVec2(115, 30)))
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
                ImGui::SetTooltip("%s", "Restore calibration in flash to factory settings");
            }
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_Text, grey);
            ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, grey);

            ImGui::Button(u8"\uF275 Restore Factory", ImVec2(115, 30));
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("%s", "Write selected calibration table to the device. For advanced users");
            }

            ImGui::PopStyleColor(2);
        }

        ImGui::PushStyleColor(ImGuiCol_ChildWindowBg, dark_sensor_bg);

        ImGui::BeginChild("##CalibData",ImVec2(w - 15, h - 110), true);

        ImGui::SetCursorPosX(10);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);

        ImGui::Text("Stereo Baseline(mm):"); ImGui::SameLine();
        ImGui::SetCursorPosX(200);

        ImGui::PushItemWidth(120);
        draw_float("Baseline", table->baseline, orig_table->baseline, changed);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
        ImGui::PopItemWidth();

        draw_float4x4("Left Intrinsics", table->intrinsic_left, orig_table->intrinsic_left, changed);
        draw_float4x4("Right Intrinsics", table->intrinsic_right, orig_table->intrinsic_right, changed);
        draw_float4x4("World to Left Rotation", table->world2left_rot, orig_table->world2left_rot, changed);
        draw_float4x4("World to Right Rotation", table->world2right_rot, orig_table->world2right_rot, changed);

        ImGui::SetCursorPosX(10);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);

        ImGui::Text("Rectified Resolution:"); ImGui::SameLine();
        ImGui::SetCursorPosX(200);

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

        ImGui::PushItemWidth(120);
        ImGui::Combo("##RectifiedResolutions", &selected_resolution, resolution_names_char.data(), int(resolution_names_char.size()));

        ImGui::SetCursorPosX(10);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);

        ImGui::Text("Focal Length:"); ImGui::SameLine();
        ImGui::SetCursorPosX(200);

        draw_float("FocalX", table->rect_params[selected_resolution].x, orig_table->rect_params[selected_resolution].x, changed);
        ImGui::SameLine();
        draw_float("FocalY", table->rect_params[selected_resolution].y, orig_table->rect_params[selected_resolution].y, changed);

        ImGui::SetCursorPosX(10);
        ImGui::Text("Principal Point:"); ImGui::SameLine();
        ImGui::SetCursorPosX(200);

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
            ImGui::SetTooltip("%s", "Changing calibration will affect depth quality. Changes are persistent.\nThere is an option to get back to factory calibration, but it maybe worse than current calibration\nBefore writing to flash, we strongly recommend to make a file backup");
        }

        ImGui::SetCursorScreenPos({ (float)(x0 + w - 230), (float)(y0 + h - 30) });

        if (ImGui::Button("Cancel", ImVec2(100, 25)))
        {
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::IsItemHovered())
        {
            window.link_hovered();
            ImGui::SetTooltip("%s", "Close without saving any changes");
        }
        ImGui::SameLine();

        auto streams = dev.query_sensors()[0].get_active_streams();
        if (_accept && streams.size())
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
                ImGui::SetTooltip("%s", "Write selected calibration table to the device");
            }
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_Text, grey);
            ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, grey);

            ImGui::Button(u8"\uF2DB  Write Table", ImVec2(120, 25));
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("%s", "Write selected calibration table to the device. For advanced users");
            }

            ImGui::PopStyleColor(2);
        }

        if (changed && streams.size())
        {
            try
            {
                dev.as<rs2::auto_calibrated_device>().set_calibration_table(_calibration);
            }
            catch (const std::exception&)
            {
                try
                {
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
