//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "ds5-private.h"
#include <iomanip>

using namespace std;

#define intrinsics_string(res) #res << "\t" << array2str((float_4&)table->rect_params[res]) << endl

namespace rsimpl
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

        rs2_intrinsics get_intrinsic_by_resolution(const vector<unsigned char> & raw_data, calibration_table_id table_id, uint32_t width, uint32_t height)
        {
            switch (table_id)
            {
            case coefficients_table_id:
            {
                if (raw_data.size() != sizeof(coefficients_table))
                {
                    throw invalid_value_exception(to_string() << "DS5 Coefficients table read error, actual size is " << raw_data.size() << " while expecting " << sizeof(coefficients_table) << " bytes");
                }
                auto table = reinterpret_cast<const coefficients_table *>(raw_data.data());
                LOG_DEBUG("DS5 Coefficients table: version [mjr.mnr]: 0x" << hex << setfill('0') << setw(4) << table->header.version << dec
                    << ", type " << table->header.table_type << ", size " << table->header.table_size
                    << ", CRC: " << hex << table->header.crc32);
                // verify the parsed table
                if (table->header.crc32 != calc_crc32(raw_data.data() + sizeof(table_header), raw_data.size() - sizeof(table_header)))
                {
                    throw invalid_value_exception("DS5 Coefficients table CRC error, parsing aborted");
                }

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
                memset(intrinsics.coeffs, 0, arr_size(intrinsics.coeffs));  // All coefficients are zeroed since rectified depth is defined as CS origin
                return intrinsics;
            }
            default:
                throw invalid_value_exception(to_string() << "Parsing Calibration table type " << table_id << " is not supported");
            }
        }

        bool try_fetch_usb_device(std::vector<uvc::usb_device_info>& devices,
                                         const uvc::uvc_device_info& info, uvc::usb_device_info& result)
        {
            for (auto it = devices.begin(); it != devices.end(); ++it)
            {
                if (it->unique_id == info.unique_id)
                {
                    result = *it;
                    switch (info.pid)
                    {
                    case RS400P_PID:
                    case RS430C_PID:
                    case RS410A_PID:
                    case RS440P_PID:
                        result.mi = 3;
                        break;
                    case RS420R_PID:
                        throw not_implemented_exception("RS420R_PID usb not implemented.");
                        break;
                    case RS450T_PID:
                        result.mi = 6;
                        break;
                    default:
                        throw not_implemented_exception("usb device not implemented.");
                        break;
                    }

                    devices.erase(it);
                    return true;
                }
            }
            return false;
        }
    } // rsimpl::ds
} // namespace rsimpl
