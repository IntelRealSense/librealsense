#include <glad/glad.h>
#include "calibration-model.h"
#include "model-views.h"
#include "os.h"

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
    if (abs(x - orig) > 0.00001) ImGui::PushStyleColor(ImGuiCol_FrameBg, regular_blue);
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

namespace helpers
{
    #define UPDC32(octet, crc) (crc_32_tab[((crc) ^ (octet)) & 0xff] ^ ((crc) >> 8))

    static const uint32_t crc_32_tab[] = { /* CRC polynomial 0xedb88320 */
        0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
        0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
        0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
        0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
        0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
        0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
        0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
        0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
        0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
        0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
        0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
        0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
        0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
        0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
        0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
        0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
        0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
        0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
        0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
        0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
        0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
        0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
        0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
        0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
        0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
        0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
        0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
        0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
        0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
        0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
        0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
        0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
        0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
        0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
        0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
        0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
        0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
        0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
        0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
        0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
        0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
        0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
        0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
    };

    /// Calculate CRC code for arbitrary characters buffer
    uint32_t calc_crc32(const uint8_t *buf, size_t bufsize)
    {
        uint32_t oldcrc32 = 0xFFFFFFFF;
        for (; bufsize; --bufsize, ++buf)
            oldcrc32 = UPDC32(*buf, oldcrc32);
        return ~oldcrc32;
    }
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
        if (ImGui::Button(u8"\uF07C Load...", ImVec2(70, 30)))
        {
            try
            {
                if (auto fn = file_dialog_open(file_dialog_mode::open_file, "Calibration JSON\0*.json\0", nullptr, nullptr))
                {
                    config_file cf(fn);
                    table->baseline = cf.get("baseline");

                    auto load_float3x4 = [&](std::string name, librealsense::float3x3& m){
                        m.x.x = cf.get(std::string(to_string() << name << ".x.x").c_str());
                        m.x.y = cf.get(std::string(to_string() << name << ".x.y").c_str());
                        m.x.z = cf.get(std::string(to_string() << name << ".x.z").c_str());

                        m.y.x = cf.get(std::string(to_string() << name << ".y.x").c_str());
                        m.y.y = cf.get(std::string(to_string() << name << ".y.y").c_str());
                        m.y.z = cf.get(std::string(to_string() << name << ".y.z").c_str());

                        m.z.x = cf.get(std::string(to_string() << name << ".z.x").c_str());
                        m.z.y = cf.get(std::string(to_string() << name << ".z.y").c_str());
                        m.z.z = cf.get(std::string(to_string() << name << ".z.z").c_str());
                    };

                    load_float3x4("intrinsic_left", table->intrinsic_left);
                    load_float3x4("intrinsic_right", table->intrinsic_right);
                    load_float3x4("world2left_rot", table->world2left_rot);
                    load_float3x4("world2right_rot", table->world2right_rot);

                    for (int i = 0; i < librealsense::ds::max_ds5_rect_resolutions; i++)
                    {
                        table->rect_params[i].x = cf.get(std::string(to_string() << "rectified." << i << ".fx").c_str());
                        table->rect_params[i].y = cf.get(std::string(to_string() << "rectified." << i << ".fy").c_str());

                        table->rect_params[i].z = cf.get(std::string(to_string() << "rectified." << i << ".ppx").c_str());
                        table->rect_params[i].w = cf.get(std::string(to_string() << "rectified." << i << ".ppy").c_str());
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
                        cf.set(std::string(to_string() << name << ".x.x").c_str(), m.x.x);
                        cf.set(std::string(to_string() << name << ".x.y").c_str(), m.x.y);
                        cf.set(std::string(to_string() << name << ".x.z").c_str(), m.x.z);

                        cf.set(std::string(to_string() << name << ".y.x").c_str(), m.y.x);
                        cf.set(std::string(to_string() << name << ".y.y").c_str(), m.y.y);
                        cf.set(std::string(to_string() << name << ".y.z").c_str(), m.y.z);

                        cf.set(std::string(to_string() << name << ".z.x").c_str(), m.z.x);
                        cf.set(std::string(to_string() << name << ".z.y").c_str(), m.z.y);
                        cf.set(std::string(to_string() << name << ".z.z").c_str(), m.z.z);
                    };

                    save_float3x4("intrinsic_left", table->intrinsic_left);
                    save_float3x4("intrinsic_right", table->intrinsic_right);
                    save_float3x4("world2left_rot", table->world2left_rot);
                    save_float3x4("world2right_rot", table->world2right_rot);
                    
                    for (int i = 0; i < librealsense::ds::max_ds5_rect_resolutions; i++)
                    {
                        auto xy = librealsense::ds::resolutions_list[(librealsense::ds::ds5_rect_resolutions)i];
                        int w = xy.x; int h = xy.y;

                        cf.set(std::string(to_string() << "rectified." << i << ".width").c_str(), w);
                        cf.set(std::string(to_string() << "rectified." << i << ".height").c_str(), h);

                        cf.set(std::string(to_string() << "rectified." << i << ".fx").c_str(), table->rect_params[i].x);
                        cf.set(std::string(to_string() << "rectified." << i << ".fy").c_str(), table->rect_params[i].y);

                        cf.set(std::string(to_string() << "rectified." << i << ".ppx").c_str(), table->rect_params[i].z);
                        cf.set(std::string(to_string() << "rectified." << i << ".ppy").c_str(), table->rect_params[i].w);
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
        for (size_t i = 0; i < resolution_offset.size(); i++)
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
                    auto crc = helpers::calc_crc32(actual_data, actual_data_size);
                    table->header.crc32 = crc;
                    dev.as<rs2::auto_calibrated_device>().set_calibration_table(_calibration);
                    dev.as<rs2::auto_calibrated_device>().write_calibration();
                    _original = _calibration;
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
