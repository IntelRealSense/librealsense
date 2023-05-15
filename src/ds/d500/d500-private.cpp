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
                return get_d500_depth_intrinsic_by_resolution(raw_data, width, height);
            }
            case d500_calibration_table_id::rgb_calibration_id:
            {
                return get_d500_color_intrinsic_by_resolution(raw_data, width, height);
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
        float4 compute_rect_params_from_resolution(const mini_intrinsics& base_intrinsics, uint32_t width, uint32_t height)
        {
            if (base_intrinsics.image_width == 0 || base_intrinsics.image_height == 0)
                throw invalid_value_exception(rsutils::string::from() << 
                    "resolution in base_intrinsics is 0: width = " << base_intrinsics.image_width <<
                    ", height = " << base_intrinsics.image_height);

            auto scale_ratio_x = static_cast<float>(width) / base_intrinsics.image_width;
            auto scale_ratio_y = static_cast<float>(height) / base_intrinsics.image_height;
            auto scale_ratio = std::max<float>(scale_ratio_x, scale_ratio_y);

            auto crop_x = (base_intrinsics.image_width * scale_ratio - width) * 0.5f;
            auto crop_y = (base_intrinsics.image_height * scale_ratio - height) * 0.5f;

            auto new_ppx = (base_intrinsics.ppx + 0.5f) * scale_ratio - crop_x - 0.5f;
            auto new_ppy = (base_intrinsics.ppy + 0.5f) * scale_ratio - crop_y - 0.5f;

            auto new_fx = base_intrinsics.fx * scale_ratio;
            auto new_fy = base_intrinsics.fy * scale_ratio;

            return { new_fx, new_fy, new_ppx, new_ppy };
        }

        rs2_intrinsics get_d500_depth_intrinsic_by_resolution(const std::vector<uint8_t>& raw_data, uint32_t width, uint32_t height)
        {
            auto table = check_calib<ds::d500_coefficients_table>(raw_data);
            if (!table)
                throw invalid_value_exception(rsutils::string::from() << "table is null");

            if (!(width > 0 && height > 0))
                throw invalid_value_exception(rsutils::string::from() << "width and height are not positive");

            rs2_intrinsics intrinsics;
            intrinsics.width = width;
            intrinsics.height = height;

            auto rect_params = compute_rect_params_from_resolution(table->left_coefficients_table.base_instrinsics, width, height);

            intrinsics.fx = rect_params[0];
            intrinsics.fy = rect_params[1];
            intrinsics.ppx = rect_params[2];
            intrinsics.ppy = rect_params[3];
            intrinsics.model = RS2_DISTORTION_BROWN_CONRADY;
            memset(intrinsics.coeffs, 0, sizeof(intrinsics.coeffs));  // All coefficients are zeroed since rectified depth is defined as CS origin

            return intrinsics;
        }

        rs2_intrinsics get_d500_color_intrinsic_by_resolution(const std::vector<uint8_t>& raw_data, uint32_t width, uint32_t height)
        {
            auto table = check_calib<ds::d500_rgb_calibration_table>(raw_data);
            if (!table)
                throw invalid_value_exception(rsutils::string::from() << "table is null");

            if (!(width > 0 && height > 0))
                throw invalid_value_exception(rsutils::string::from() << "width and height are not positive");

            rs2_intrinsics intrinsics;
            intrinsics.width = width;
            intrinsics.height = height;

            auto rect_params = compute_rect_params_from_resolution(table->rgb_coefficients_table.base_instrinsics, width, height);

            intrinsics.fx = rect_params[0];
            intrinsics.fy = rect_params[1];
            intrinsics.ppx = rect_params[2];
            intrinsics.ppy = rect_params[3];
            intrinsics.model = table->rgb_coefficients_table.distortion_model;
            librealsense::copy(intrinsics.coeffs, table->rgb_coefficients_table.distortion_coeffs, sizeof(intrinsics.coeffs));


            return intrinsics;
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
