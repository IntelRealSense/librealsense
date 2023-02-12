//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include "d500-private.h"

using namespace std;

namespace librealsense
{
    namespace ds
    {
        bool d500_try_fetch_usb_device(std::vector<platform::usb_device_info>& devices,
            const platform::uvc_device_info& info, platform::usb_device_info& result)
        {
            for (auto it = devices.begin(); it != devices.end(); ++it)
            {
                if (it->unique_id == info.unique_id)
                {
                    bool found = false;
                    result = *it;
                    switch (info.pid)
                    {
                    case RS_D585_PID:
                    case RS_D585S_PID:
                        found = (result.mi == 6);
                        break;
                    default:
                        throw not_implemented_exception(rsutils::string::from() << "USB device "
                            << std::hex << info.pid << ":" << info.vid << std::dec << " is not supported.");
                        break;
                    }

                    if (found)
                    {
                        devices.erase(it);
                        return true;
                    }
                }
            }
            return false;
        }

        rs2_intrinsics get_d500_intrinsic_by_resolution(const vector<uint8_t>& raw_data, d500_calibration_table_id table_id, uint32_t width, uint32_t height)
        {
            switch (table_id)
            {
            case d500_calibration_table_id::depth_calibration_id:
            {
                return get_d500_intrinsic_by_resolution_coefficients_table(raw_data, width, height);
            }
            case d500_calibration_table_id::rgb_calibration_id:
            {
                return get_color_stream_intrinsic(raw_data, width, height);
            }
            default:
                throw invalid_value_exception(rsutils::string::from() << "Parsing Calibration table type " << static_cast<int>(table_id) << " is not supported");
            }
        }

        // Algorithm prepared by Oscar Pelc in matlab:
        // calib_new.image_size = new_size_xy;
        // scale_ratio = max(new_size_xy . / calib_org.image_size);
        // crop_xy = (calib_org.image_size * scale_ratio - new_size_xy) / 2;
        // calib_new.principal_point = (calib_org.principal_point + 0.5).*scale_ratio - crop_xy - 0.5;
        // calib_new.focal_length = calib_org.focal_length.*scale_ratio;
        float4 compute_rect_params_from_resolution(const ds::d500_coefficients_table* table, uint32_t width, uint32_t height)
        {
            if (!table)
                throw invalid_value_exception(rsutils::string::from() << "table is null");
            if (table->left_coefficients_table.base_instrinsics.image_width == 0 ||
                table->left_coefficients_table.base_instrinsics.image_height == 0)
                throw invalid_value_exception(rsutils::string::from() << 
                    "resolution in table->left_coefficients_table.base_instrinsics is 0: width = " << 
                    table->left_coefficients_table.base_instrinsics.image_width <<
                    ", height = " << table->left_coefficients_table.base_instrinsics.image_height);

            auto scale_ratio_x = static_cast<float>(width) / table->left_coefficients_table.base_instrinsics.image_width;
            auto scale_ratio_y = static_cast<float>(height) / table->left_coefficients_table.base_instrinsics.image_height;
            auto scale_ratio = std::max<float>(scale_ratio_x, scale_ratio_y);

            auto crop_x = (table->left_coefficients_table.base_instrinsics.image_width * scale_ratio - width) * 0.5f;
            auto crop_y = (table->left_coefficients_table.base_instrinsics.image_height * scale_ratio - height) * 0.5f;

            auto new_ppx = (table->left_coefficients_table.base_instrinsics.ppx + 0.5f) * scale_ratio - crop_x - 0.5f;
            auto new_ppy = (table->left_coefficients_table.base_instrinsics.ppy + 0.5f) * scale_ratio - crop_y - 0.5f;

            auto new_fx = table->left_coefficients_table.base_instrinsics.fx * scale_ratio;
            auto new_fy = table->left_coefficients_table.base_instrinsics.fy * scale_ratio;

            return { new_fx, new_fy, new_ppx, new_ppy };
        }

        rs2_intrinsics get_d500_intrinsic_by_resolution_coefficients_table(const std::vector<uint8_t>& raw_data, uint32_t width, uint32_t height)
        {
            auto table = check_calib<ds::d500_coefficients_table>(raw_data);

            if (width > 0 && height > 0)
            {
                rs2_intrinsics intrinsics;
                intrinsics.width = width;
                intrinsics.height = height;

                auto rect_params = compute_rect_params_from_resolution(table, width, height);

                // DS5U - assume ideal intrinsic params
                if ((rect_params.x == rect_params.y) && (rect_params.z == rect_params.w))
                {
                    rect_params.x = rect_params.y = intrinsics.width * 1.5f;
                    rect_params.z = intrinsics.width * 0.5f;
                    rect_params.w = intrinsics.height * 0.5f;
                }
                intrinsics.fx = rect_params[0];
                intrinsics.fy = rect_params[1];
                intrinsics.ppx = rect_params[2];
                intrinsics.ppy = rect_params[3];
                intrinsics.model = RS2_DISTORTION_BROWN_CONRADY;
                memset(intrinsics.coeffs, 0, sizeof(intrinsics.coeffs));  // All coefficients are zeroed since rectified depth is defined as CS origin

                // In case of the special 848x100 resolution adjust the intrinsics
                if (width == 848 && height == 100)
                {
                    intrinsics.height = 100;
                    intrinsics.ppy -= 190;
                }

                return intrinsics;
            }
            else
            {
                // Intrinsics not found in the calibration table - use the generic calculation
                ds_rect_resolutions resolution = res_1920_1080;
                rs2_intrinsics intrinsics;
                intrinsics.width = width;
                intrinsics.height = height;

                auto rect_params = float4(); // TODO - REMI static_cast<const float4>();  table->ds_rect_params[resolution]);
                // DS5U - assume ideal intrinsic params
                if ((rect_params.x == rect_params.y) && (rect_params.z == rect_params.w))
                {
                    rect_params.x = rect_params.y = intrinsics.width * 1.5f;
                    rect_params.z = intrinsics.width * 0.5f;
                    rect_params.w = intrinsics.height * 0.5f;
                }

                // Special resolution for auto-calibration requires special treatment...
                if (width == 256 && height == 144)
                {
                    intrinsics.fx = rect_params[0];
                    intrinsics.fy = rect_params[1];
                    intrinsics.ppx = rect_params[2] - 832;
                    intrinsics.ppy = rect_params[3] - 468;
                }
                else
                {
                    intrinsics.fx = rect_params[0] * width / resolutions_list[resolution].x;
                    intrinsics.fy = rect_params[1] * height / resolutions_list[resolution].y;
                    intrinsics.ppx = rect_params[2] * width / resolutions_list[resolution].x;
                    intrinsics.ppy = rect_params[3] * height / resolutions_list[resolution].y;
                }

                intrinsics.model = RS2_DISTORTION_BROWN_CONRADY;
                memset(intrinsics.coeffs, 0, sizeof(intrinsics.coeffs));  // All coefficients are zeroed since rectified depth is defined as CS origin

                return intrinsics;
            }
        }

        pose get_d500_color_stream_extrinsic(const std::vector<uint8_t>& raw_data)
        {
            auto table = check_calib<d500_rgb_calibration_table>(raw_data);
            float3 trans_vector = table->translation_rect;
            
            float3x3 rect_rot_mat = table->rgb_coefficients_table.rotation_matrix;
            float trans_scale = -0.001f; // Convert units from mm to meter. Extrinsic of color is referenced to the Depth Sensor CS

            trans_vector.x *= trans_scale;
            trans_vector.y *= trans_scale;
            trans_vector.z *= trans_scale;

            return{ rect_rot_mat,trans_vector };
        }
    } // librealsense::ds
} // namespace librealsense
