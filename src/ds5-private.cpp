//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "ds5-private.h"

using namespace std;

#define intrinsics_string(res) #res << "\t" << array2str((float_4&)table->rect_params[res]) << endl

namespace rsimpl2
{
    namespace ds
    {
        ds5_rect_resolutions width_height_to_ds5_rect_resolutions(uint32_t width, uint32_t height)
        {
            for (auto& elem : resolutions_list)
            {
                if (elem.second.x == width && elem.second.y == height)
                    return elem.first;
            }
            throw invalid_value_exception("resolution not found.");
        }

        rs2_intrinsics get_intrinsic_by_resolution_coefficients_table(const std::vector<uint8_t> & raw_data, uint32_t width, uint32_t height)
        {
            auto table = check_calib<ds::coefficients_table>(raw_data);

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
                << intrinsics_string(res_480_270));

            auto resolution = width_height_to_ds5_rect_resolutions(width ,height);
            rs2_intrinsics intrinsics;
            intrinsics.width = resolutions_list[resolution].x;
            intrinsics.height = resolutions_list[resolution].y;

            auto rect_params = static_cast<const float4>(table->rect_params[resolution]);
            intrinsics.fx = rect_params[0];
            intrinsics.fy = rect_params[1];
            intrinsics.ppx = rect_params[2];
            intrinsics.ppy = rect_params[3];
            intrinsics.model = RS2_DISTORTION_BROWN_CONRADY;
            memset(intrinsics.coeffs, 0, sizeof(intrinsics.coeffs));  // All coefficients are zeroed since rectified depth is defined as CS origin
            return intrinsics;
        }

        rs2_intrinsics get_intrinsic_fisheye_table(const std::vector<uint8_t>& raw_data, uint32_t width, uint32_t height)
        { 
             auto table = check_calib<ds::fisheye_intrinsics_table>(raw_data);

             rs2_intrinsics intrinsics;
             auto intrin = table->intrinsic;
             intrinsics.fx = intrin(0,0);
             intrinsics.fy = intrin(1,1);
             intrinsics.ppx = intrin(2,0);
             intrinsics.ppy = intrin(2,1);
             intrinsics.model = RS2_DISTORTION_FTHETA;
             rsimpl2::copy(intrinsics.coeffs, table->distortion, sizeof(table->distortion));


             LOG_DEBUG(endl<<
                       array2str((float_4&)(intrinsics.fx, intrinsics.fy, intrinsics.ppx, intrinsics.ppy)) << endl);

             return intrinsics;
        }

        rs2_intrinsics get_intrinsic_by_resolution(const vector<uint8_t> & raw_data, calibration_table_id table_id, uint32_t width, uint32_t height)
        {
            switch (table_id)
            {
            case coefficients_table_id:
            {
                return get_intrinsic_by_resolution_coefficients_table(raw_data, width, height);
            }
            case fisheye_calibration_id:
            {
                return get_intrinsic_fisheye_table(raw_data, width, height);
            }
            default:
                throw invalid_value_exception(to_string() << "Parsing Calibration table type " << table_id << " is not supported");
            }
        }

        pose get_fisheye_extrinsics_data(const vector<uint8_t> & raw_data)
        {
            auto table = check_calib<fisheye_extrinsics_table>(raw_data);

            auto rot = table->rotation;
            auto trans = table->translation;

            pose ex = {{rot(0,0), rot(1,0),rot(2,0),rot(1,0), rot(1,1),rot(2,1),rot(0,2), rot(1,2),rot(2,2)},
                       {trans[0], trans[1], trans[2]}};
            return ex;
        }



        bool try_fetch_usb_device(std::vector<uvc::usb_device_info>& devices,
                                         const uvc::uvc_device_info& info, uvc::usb_device_info& result)
        {
            for (auto it = devices.begin(); it != devices.end(); ++it)
            {
                if (it->unique_id == info.unique_id)
                {
                    bool found = false;
                    result = *it;
                    switch (info.pid)
                    {
                    case RS400_PID:
                    case RS410_PID:
                    case RS430_PID:
                    case RS420_PID:
                        found = result.mi == 3;
                        break;
                    case RS415_PID:
                        result.mi = 4;
                        break;
                    case RS430_MM_PID:
                    case RS420_MM_PID:
                        found = result.mi == 6;
                        break;
                    default:
                        throw not_implemented_exception("usb device not implemented.");
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
    } // rsimpl::ds
} // namespace rsimpl
