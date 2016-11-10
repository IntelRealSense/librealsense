//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "ds5-private.h"
#include <iomanip>

using namespace std;

#define stringify( name ) # name "\t"

namespace rsimpl {
    namespace ds {

        void get_calibration_table_entry(const hw_monitor& hw_mon, calibration_table_id table_id, vector<uint8_t>& raw_data)
        {
            command cmd(GETINTCAL, table_id);
            raw_data = hw_mon.send(cmd);
        }

        template<typename T, int sz>
        int arr_size(T(&)[sz])
        {
            return sz;
        }

        template<typename T>
        const string array2str(T& data)
        {
            stringstream ss;
            for (int i = 0; i < arr_size(data); i++)
                ss << " [" << i << "] = " << data[i] << "\t";
            return string(ss.str().c_str());
        }

        typedef float float_4[4];

        rs_intrinsics get_ds5_intrinsic_by_resolution(const vector<unsigned char> & raw_data, calibration_table_id table_id, uint32_t width, uint32_t height)
        {
            switch (table_id)
            {
            case coefficients_table_id:
            {
                if (raw_data.size() != sizeof(coefficients_table))
                {
                    stringstream ss; ss << "DS5 Coefficients table read error, actual size is " << raw_data.size() << " while expecting " << sizeof(coefficients_table) << " bytes";
                    string str(ss.str().c_str());
                    LOG_ERROR(str);
                    throw runtime_error(str);
                }
                auto table = reinterpret_cast<const coefficients_table *>(raw_data.data());
                LOG_DEBUG("DS5 Coefficients table: version [mjr.mnr]: 0x" << hex << setfill('0') << setw(4) << table->header.version << dec
                    << ", type " << table->header.table_type << ", size " << table->header.table_size
                    << ", CRC: " << hex << table->header.crc32);
                // verify the parsed table
                if (table->header.crc32 != calc_crc32(raw_data.data() + sizeof(table_header), raw_data.size() - sizeof(table_header)))
                {
                    string str("DS5 Coefficients table CRC error, parsing aborted");
                    LOG_ERROR(str);
                    throw runtime_error(str);
                }

                LOG_DEBUG(endl
                    << "baseline = " << table->baseline << " mm" << endl
                    << "Rect params:  \t fX\t\t fY\t\t ppX\t\t ppY \n"
                    << stringify(res_1920_1080) << array2str((float_4&)table->rect_params[res_1920_1080]) << endl
                    << stringify(res_1280_720) << array2str((float_4&)table->rect_params[res_1280_720]) << endl
                    << stringify(res_640_480) << array2str((float_4&)table->rect_params[res_640_480]) << endl
                    << stringify(res_848_480) << array2str((float_4&)table->rect_params[res_848_480]) << endl
                    << stringify(res_424_240) << array2str((float_4&)table->rect_params[res_424_240]) << endl
                    << stringify(res_640_360) << array2str((float_4&)table->rect_params[res_640_360]) << endl
                    << stringify(res_320_240) << array2str((float_4&)table->rect_params[res_320_240]) << endl
                    << stringify(res_480_270) << array2str((float_4&)table->rect_params[res_480_270]));

                auto resolution = width_height_to_ds5_rect_resolutions(width ,height);
                rs_intrinsics intrinsics;
                intrinsics.width = resolutions_list[resolution].x;
                intrinsics.height = resolutions_list[resolution].y;

                auto rect_params = static_cast<const float4>(table->rect_params[resolution]);
                intrinsics.fx = rect_params[0];
                intrinsics.fy = rect_params[1];
                intrinsics.ppx = rect_params[2];
                intrinsics.ppy = rect_params[3];
                intrinsics.model = RS_DISTORTION_BROWN_CONRADY;
                memset(intrinsics.coeffs, 0, arr_size(intrinsics.coeffs));  // All coefficients are zeroed since rectified depth is defined as CS origin
                return intrinsics;
            }
            default:
                LOG_ERROR("Parsing Calibration table type " << table_id << " is not supported");
                throw runtime_error(to_string() << "Parsing Calibration table type " << table_id << " is not supported");
            }
        }

        void get_ds5_table_raw_data(const hw_monitor& hw_mon, calibration_table_id table_id, vector<uint8_t>& table_raw_data)
        {
            try
            {
                get_calibration_table_entry(hw_mon, table_id, table_raw_data);
            }
            catch (const runtime_error &e)
            {
                LOG_ERROR(e.what());
            }
            catch (...)
            {
                LOG_ERROR("Reading DS5 Calibration failed, table " << table_id);
            }
        }

    } // rsimpl::ds
} // namespace rsimpl