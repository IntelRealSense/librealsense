//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include "ds-private.h"

using namespace std;


namespace librealsense
{
    namespace ds
    {
        ds_rect_resolutions width_height_to_ds_rect_resolutions(uint32_t width, uint32_t height)
        {
            for (auto& elem : resolutions_list)
            {
                if (uint32_t(elem.second.x) == width && uint32_t(elem.second.y) == height)
                    return elem.first;
            }
            return max_ds_rect_resolutions;
        }

        rs2_intrinsics get_intrinsic_fisheye_table(const std::vector<uint8_t>& raw_data, uint32_t width, uint32_t height)
        {
            auto table = check_calib<ds::fisheye_calibration_table>(raw_data);

            rs2_intrinsics intrinsics;
            auto intrin = table->intrinsic;
            intrinsics.fx = intrin(0, 0);
            intrinsics.fy = intrin(1, 1);
            intrinsics.ppx = intrin(2, 0);
            intrinsics.ppy = intrin(2, 1);
            intrinsics.model = RS2_DISTORTION_FTHETA;

            intrinsics.height = height;
            intrinsics.width = width;

            librealsense::copy(intrinsics.coeffs, table->distortion, sizeof(table->distortion));

            LOG_DEBUG(endl << array2str((float_4&)(intrinsics.fx, intrinsics.fy, intrinsics.ppx, intrinsics.ppy)) << endl);

            return intrinsics;
        }

        rs2_intrinsics get_color_stream_intrinsic(const std::vector<uint8_t>& raw_data, uint32_t width, uint32_t height)
        {
            auto table = check_calib<ds::rgb_calibration_table>(raw_data);

            // Compensate for aspect ratio as the normalized intrinsic is calculated with a single resolution
            float3x3 intrin = table->intrinsic;
            float calib_aspect_ratio = 9.f / 16.f; // shall be overwritten with the actual calib resolution

            if (table->calib_width && table->calib_height)
                calib_aspect_ratio = float(table->calib_height) / float(table->calib_width);
            else
            {
                LOG_WARNING("RGB Calibration resolution is not specified, using default 16/9 Aspect ratio");
            }

            // Compensate for aspect ratio
            float actual_aspect_ratio = height / (float)width;
            if (actual_aspect_ratio < calib_aspect_ratio)
            {
                intrin(1, 1) *= calib_aspect_ratio / actual_aspect_ratio;
                intrin(2, 1) *= calib_aspect_ratio / actual_aspect_ratio;
            }
            else
            {
                intrin(0, 0) *= actual_aspect_ratio / calib_aspect_ratio;
                intrin(2, 0) *= actual_aspect_ratio / calib_aspect_ratio;
            }

            // Calculate specific intrinsic parameters based on the normalized intrinsic and the sensor's resolution
            rs2_intrinsics calc_intrinsic{
                static_cast<int>(width),
                static_cast<int>(height),
                ((1 + intrin(2, 0)) * width) / 2.f,
                ((1 + intrin(2, 1)) * height) / 2.f,
                intrin(0, 0) * width / 2.f,
                intrin(1, 1) * height / 2.f,
                RS2_DISTORTION_INVERSE_BROWN_CONRADY  // The coefficients shall be use for undistort
            };
            librealsense::copy(calc_intrinsic.coeffs, table->distortion, sizeof(table->distortion));
            LOG_DEBUG(endl << array2str((float_4&)(calc_intrinsic.fx, calc_intrinsic.fy, calc_intrinsic.ppx, calc_intrinsic.ppy)) << endl);

            static rs2_intrinsics ref{};
            if (memcmp(&calc_intrinsic, &ref, sizeof(rs2_intrinsics)))
            {
                LOG_DEBUG_THERMAL_LOOP("RGB Intrinsic: ScaleX, ScaleY = "
                    << std::setprecision(3) << intrin(0, 0) << ", " << intrin(1, 1)
                    << ". Fx,Fy = " << calc_intrinsic.fx << "," << calc_intrinsic.fy);
                ref = calc_intrinsic;
            }

            return calc_intrinsic;
        }

        // Parse intrinsics from newly added RECPARAMSGET command
        bool try_get_intrinsic_by_resolution_new(const vector<uint8_t>& raw_data,
            uint32_t width, uint32_t height, rs2_intrinsics* result)
        {
            using namespace ds;
            auto count = raw_data.size() / sizeof(new_calibration_item);
            auto items = (new_calibration_item*)raw_data.data();
            for (int i = 0; i < count; i++)
            {
                auto&& item = items[i];
                if (item.width == width && item.height == height)
                {
                    result->width = width;
                    result->height = height;
                    result->ppx = item.ppx;
                    result->ppy = item.ppy;
                    result->fx = item.fx;
                    result->fy = item.fy;
                    result->model = RS2_DISTORTION_BROWN_CONRADY;
                    memset(result->coeffs, 0, sizeof(result->coeffs));  // All coefficients are zeroed since rectified depth is defined as CS origin

                    return true;
                }
            }
            return false;
        }

        pose get_fisheye_extrinsics_data(const vector<uint8_t>& raw_data)
        {
            auto table = check_calib<fisheye_extrinsics_table>(raw_data);

            auto rot = table->rotation;
            auto trans = table->translation;

            pose ex = { {rot(0,0), rot(1,0),rot(2,0),rot(1,0), rot(1,1),rot(2,1),rot(0,2), rot(1,2),rot(2,2)},
                       {trans[0], trans[1], trans[2]} };
            return ex;
        }

        pose get_color_stream_extrinsic(const std::vector<uint8_t>& raw_data)
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

        flash_structure get_rw_flash_structure(const uint32_t flash_version)
        {
            switch (flash_version)
            {
                // { number of payloads in section { ro table types }} see Flash.xml
            case 100: return { 2, { 17, 10, 40, 29, 30, 54} };
            case 101: return { 3, { 10, 16, 40, 29, 18, 19, 30, 20, 21, 54 } };
            case 102: return { 3, { 9, 10, 16, 40, 29, 18, 19, 30, 20, 21, 54 } };
            case 103: return { 4, { 9, 10, 16, 40, 29, 18, 19, 30, 20, 21, 54 } };
            case 104: return { 4, { 9, 10, 40, 29, 18, 19, 30, 20, 21, 54 } };
            case 105: return { 5, { 9, 10, 40, 29, 18, 19, 30, 20, 21, 54 } };
            case 106: return { 5, { 15, 9, 10, 16, 40, 29, 18, 19, 30, 20, 21, 54 } };
            case 107: return { 6, { 15, 9, 10, 16, 40, 29, 18, 19, 30, 20, 21, 54 } };
            default:
                throw std::runtime_error("Unsupported flash version: " + std::to_string(flash_version));
            }
        }

        flash_structure get_ro_flash_structure(const uint32_t flash_version)
        {
            switch (flash_version)
            {
                // { number of payloads in section { ro table types }} see Flash.xml
            case 100: return { 2, { 134, 25 } };
            default:
                throw std::runtime_error("Unsupported flash version: " + std::to_string(flash_version));
            }
        }

        flash_info get_flash_info(const std::vector<uint8_t>& flash_buffer)
        {
            flash_info rv = {};

            uint32_t header_offset = FLASH_INFO_HEADER_OFFSET;
            memcpy(&rv.header, flash_buffer.data() + header_offset, sizeof(rv.header));

            uint32_t ro_toc_offset = FLASH_RO_TABLE_OF_CONTENT_OFFSET;
            uint32_t rw_toc_offset = FLASH_RW_TABLE_OF_CONTENT_OFFSET;

            auto ro_toc = parse_table_of_contents(flash_buffer, ro_toc_offset);
            auto rw_toc = parse_table_of_contents(flash_buffer, rw_toc_offset);

            auto ro_structure = get_ro_flash_structure(ro_toc.header.version);
            auto rw_structure = get_rw_flash_structure(rw_toc.header.version);

            rv.read_only_section = parse_flash_section(flash_buffer, ro_toc, ro_structure);
            rv.read_only_section.offset = rv.header.read_only_start_address;
            rv.read_write_section = parse_flash_section(flash_buffer, rw_toc, rw_structure);
            rv.read_write_section.offset = rv.header.read_write_start_address;

            return rv;
        }
    } // librealsense::ds
} // namespace librealsense