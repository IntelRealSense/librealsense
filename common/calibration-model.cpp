#include <glad/glad.h>
#include "calibration-model.h"
#include "model-views.h"

#include "../src/ds5/ds5-private.h"

using namespace rs2;

bool calibration_model::supports()
{
    bool is_d400 = dev.supports(RS2_CAMERA_INFO_PRODUCT_LINE) ?
        std::string(dev.get_info(RS2_CAMERA_INFO_PRODUCT_LINE)) == "D400" : false;

    return dev.is<rs2::auto_calibrated_device>() && is_d400;
}

calibration_model::calibration_model(rs2::device dev) : dev(dev) 
{
    _accept = config_file::instance().get_or_default(configurations::calibration::enable_writing, false);
}

void calibration_model::draw_float(std::string name, float& x, const float& orig, bool& changed)
{
    if (x != orig) ImGui::PushStyleColor(ImGuiCol_FrameBg, regular_blue);
    else ImGui::PushStyleColor(ImGuiCol_FrameBg, black);
    if (ImGui::DragFloat(std::string(to_string() << "##" << name).c_str(), &x, 0.001f))
    {
        changed = true;
    }
    ImGui::PopStyleColor();
}

void calibration_model::draw_float4x4(std::string name, librealsense::float3x3& feild, 
                                      const librealsense::float3x3& original, bool& changed)
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

    auto table = (librealsense::ds::coefficients_table*)_calibration.data();
    auto orig_table = (librealsense::ds::coefficients_table*)_original.data();
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
        ImGui::Button(u8"\uF07C Load...", ImVec2(70, 30)); 
        if (ImGui::IsItemHovered())
        {
            window.link_hovered();
            ImGui::SetTooltip("%s", "Load calibration from file");
        }
        ImGui::SameLine();
        ImGui::Button(u8"\uF0C7 Save As...", ImVec2(100, 30)); 
        if (ImGui::IsItemHovered())
        {
            window.link_hovered();
            ImGui::SetTooltip("%s", "Save calibration image to file");
        }
        ImGui::SameLine();
        if (_accept)
        {
            if (ImGui::Button(u8"\uF275 Restore Factory", ImVec2(110, 30)))
            {
                try
                {
                    dev.as<rs2::auto_calibrated_device>().reset_to_factory_calibration();
                    _calibration = dev.as<rs2::auto_calibrated_device>().get_calibration_table();
                    _original = _calibration;
                    changed = true;
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

            ImGui::Button(u8"\uF275 Restore Factory", ImVec2(110, 30));
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
        for (int i = 0; i < librealsense::ds::max_ds5_rect_resolutions; i++)
        {
            auto xy = librealsense::ds::resolutions_list[(librealsense::ds::ds5_rect_resolutions)i];
            int w = xy.x; int h = xy.y;
            if (w != 0) {
                resolution_offset.push_back(i);
                std::string name = to_string() << w <<" x "<<h;
                resolution_names.push_back(name);
            }
        }
        for (int i = 0; i < resolution_offset.size(); i++)
        {
            resolution_names_char.push_back(resolution_names[i].c_str());
        }

        ImGui::PushItemWidth(120);
        ImGui::Combo("##RectifiedResolutions", &selected_resolution, resolution_names_char.data(), resolution_names_char.size());

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

        if (_accept)
        {
            if (ImGui::Button(u8"\uF2DB  Write Table", ImVec2(120, 25)))
            {
                try
                {
                    dev.as<rs2::auto_calibrated_device>().set_calibration_table(_calibration);
                    dev.as<rs2::auto_calibrated_device>().write_calibration();
                    _original = _calibration;
                }
                catch (const std::exception& ex)
                {
                    error_message = ex.what();
                    ImGui::CloseCurrentPopup();
                }
                
                ImGui::CloseCurrentPopup();
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

        auto streams = dev.query_sensors()[0].get_active_streams();
        if (changed && streams.size())
        {
            try
            {
                dev.as<rs2::auto_calibrated_device>().set_calibration_table(_calibration);
            }
            catch (const std::exception& ex)
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

        ImGui::EndPopup();
    }
    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(2);
}