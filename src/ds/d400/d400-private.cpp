//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "d400-private.h"

using namespace std;

#define intrinsics_string(res) #res << "\t" << array2str((float_4&)table->rect_params[res]) << endl

namespace librealsense
{
    namespace ds
    {
        bool d400_try_fetch_usb_device(std::vector<platform::usb_device_info>& devices,
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
                    case RS_USB2_PID:
                    case RS400_PID:
                    case RS405U_PID:
                    case RS410_PID:
                    case RS416_PID:
                    case RS460_PID:
                    case RS430_PID:
                    case RS420_PID:
                    case RS400_IMU_PID:
                        found = (result.mi == 3);
                        break;
                    case RS405_PID:
                    case RS430I_PID:
                        found = (result.mi == 4);
                        break;
                    case RS430_MM_PID:
                    case RS420_MM_PID:
                    case RS435I_PID:
                    case RS455_PID:
                        found = (result.mi == 6);
                        break;
                    case RS415_PID:
                    case RS416_RGB_PID:
                    case RS435_RGB_PID:
                    case RS465_PID:
                        found = (result.mi == 5);
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

        std::vector<platform::uvc_device_info> filter_d400_device_by_capability(const std::vector<platform::uvc_device_info>& devices,
            ds_caps caps)
        {
            std::vector<platform::uvc_device_info> results;

            switch (caps)
            {
            case ds_caps::CAP_FISHEYE_SENSOR:
                std::copy_if(devices.begin(), devices.end(), std::back_inserter(results),
                    [](const platform::uvc_device_info& info)
                    {
                        return d400_fisheye_pid.find(info.pid) != d400_fisheye_pid.end();
                    });
                break;
            default:
                throw invalid_value_exception(rsutils::string::from()
                    << "Capability filters are not implemented for val "
                    << std::hex << caps << std::dec);
            }
            return results;
        }

        rs2_intrinsics get_d400_intrinsic_by_resolution(const vector<uint8_t>& raw_data, d400_calibration_table_id table_id, uint32_t width, uint32_t height)
        {
            switch (table_id)
            {
            case d400_calibration_table_id::coefficients_table_id:
            {
                return get_d400_intrinsic_by_resolution_coefficients_table(raw_data, width, height);
            }
            case d400_calibration_table_id::fisheye_calibration_id:
            {
                return get_intrinsic_fisheye_table(raw_data, width, height);
            }
            case d400_calibration_table_id::rgb_calibration_id:
            {
                return get_color_stream_intrinsic(raw_data, width, height);
            }
            default:
                throw invalid_value_exception(rsutils::string::from() << "Parsing Calibration table type " << static_cast<int>(table_id) << " is not supported");
            }
        }

        rs2_intrinsics get_d400_intrinsic_by_resolution_coefficients_table(const std::vector<uint8_t>& raw_data, uint32_t width, uint32_t height)
        {
            auto table = check_calib<ds::d400_coefficients_table>(raw_data);

            LOG_DEBUG(endl
                << "baseline = " << table->baseline << " mm" << endl
                << "Rect params:  \t fX\t\t fY\t\t ppX\t\t ppY \n"
                << intrinsics_string(res_1920_1080)
                << intrinsics_string(res_1280_720)
                << intrinsics_string(res_640_480)
                << intrinsics_string(res_848_480)
                << intrinsics_string(res_424_240)
                << intrinsics_string(res_640_360)
                << intrinsics_string(res_320_240)
                << intrinsics_string(res_480_270)
                << intrinsics_string(res_1280_800)
                << intrinsics_string(res_960_540));

            auto resolution = width_height_to_ds_rect_resolutions(width, height);

            if (width == 848 && height == 100) // Special 848x100 resolution is available in some firmware versions
                // for this resolution we use the same base-intrinsics as for 848x480 and adjust them later
            {
                resolution = width_height_to_ds_rect_resolutions(width, 480);
            }

            if (resolution < max_ds_rect_resolutions)
            {
                rs2_intrinsics intrinsics;
                intrinsics.width = resolutions_list[resolution].x;
                intrinsics.height = resolutions_list[resolution].y;

                auto rect_params = static_cast<const float4>(table->rect_params[resolution]);
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

                auto rect_params = static_cast<const float4>(table->rect_params[resolution]);
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

        pose get_d400_color_stream_extrinsic(const std::vector<uint8_t>& raw_data)
        {
            auto table = check_calib<rgb_calibration_table>(raw_data);
            float3 trans_vector = table->translation_rect;
            float3x3 rect_rot_mat = table->rotation_matrix_rect;
            float trans_scale = -0.001f; // Convert units from mm to meter. Extrinsic of color is referenced to the Depth Sensor CS

            trans_vector.x *= trans_scale;
            trans_vector.y *= trans_scale;
            trans_vector.z *= trans_scale;

            return{ rect_rot_mat,trans_vector };
        }
    } // librealsense::ds
} // namespace librealsense