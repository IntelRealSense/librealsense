//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include "d500-private.h"
#include <src/platform/uvc-device-info.h>

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
                    // to be uncommented after d500 device is added
                    /*switch (info.pid)
                    {

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
                    */
                }
            }
            return false;
        }

        rs2_intrinsics get_d500_intrinsic_by_resolution(const vector<uint8_t>& raw_data, d500_calibration_table_id table_id, 
            uint32_t width, uint32_t height, bool is_symmetrization_enabled)
        {
            switch (table_id)
            {
            case d500_calibration_table_id::depth_calibration_id:
            {
                return get_d500_depth_intrinsic_by_resolution(raw_data, width, height, is_symmetrization_enabled);
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
        float4 compute_rect_params_from_resolution(const mini_intrinsics& base_intrinsics, uint32_t width, uint32_t height, bool force_symetry = false)
        {
            if (base_intrinsics.image_width == 0 || base_intrinsics.image_height == 0)
                throw invalid_value_exception(rsutils::string::from() << 
                    "resolution in base_intrinsics is 0: width = " << base_intrinsics.image_width <<
                    ", height = " << base_intrinsics.image_height);

            mini_intrinsics relevant_intrinsics = base_intrinsics;

            // cropping the frame in case symetry is needed, so that the principal point is at the middle
            if (force_symetry)
            {
                relevant_intrinsics.image_width = 1 + 2 * 
                    static_cast<int>(std::min<float>(base_intrinsics.ppx, base_intrinsics.image_width - 1 - base_intrinsics.ppx));
                relevant_intrinsics.image_height = 1 + 2 * 
                    static_cast<int>(std::min<float>(base_intrinsics.ppy, base_intrinsics.image_height - 1 - base_intrinsics.ppy));
                relevant_intrinsics.ppx = (relevant_intrinsics.image_width - 1) / 2.f;
                relevant_intrinsics.ppy = (relevant_intrinsics.image_height - 1) / 2.f;
            }

            auto scale_ratio_x = static_cast<float>(width) / relevant_intrinsics.image_width;
            auto scale_ratio_y = static_cast<float>(height) / relevant_intrinsics.image_height;
            auto scale_ratio = std::max<float>(scale_ratio_x, scale_ratio_y);

            auto crop_x = (relevant_intrinsics.image_width * scale_ratio - width) * 0.5f;
            auto crop_y = (relevant_intrinsics.image_height * scale_ratio - height) * 0.5f;

            auto new_ppx = (relevant_intrinsics.ppx + 0.5f) * scale_ratio - crop_x - 0.5f;
            auto new_ppy = (relevant_intrinsics.ppy + 0.5f) * scale_ratio - crop_y - 0.5f;

            auto new_fx = relevant_intrinsics.fx * scale_ratio;
            auto new_fy = relevant_intrinsics.fy * scale_ratio;

            return { new_fx, new_fy, new_ppx, new_ppy };
        }

        rs2_intrinsics get_d500_depth_intrinsic_by_resolution(const std::vector<uint8_t>& raw_data, uint32_t width, 
            uint32_t height, bool is_symmetrization_enabled)
        {
            auto table = check_calib<ds::d500_coefficients_table>(raw_data);
            if (!table)
                throw invalid_value_exception(rsutils::string::from() << "table is null");

            if (!(width > 0 && height > 0))
                throw invalid_value_exception(rsutils::string::from() << "width and height are not positive");

            rs2_intrinsics intrinsics;
            intrinsics.width = width;
            intrinsics.height = height;

            auto rect_params = compute_rect_params_from_resolution(table->rectified_intrinsics, 
                width, height, is_symmetrization_enabled);

            intrinsics.fx = rect_params[0];
            intrinsics.fy = rect_params[1];
            intrinsics.ppx = rect_params[2];
            intrinsics.ppy = rect_params[3];
            intrinsics.model = RS2_DISTORTION_BROWN_CONRADY;
            memset(intrinsics.coeffs, 0, sizeof(intrinsics.coeffs));  // All coefficients are zeroed since rectified depth is defined as CS origin

            return intrinsics;
        }


        // Algorithm prepared by Oscar Pelc in matlab:
        // % ### Adapt the calibration table to the undistorted image
        //     calib_undist = calib_sensor;
        // % # The intrinsic parameters
        //     % #      The intrinsics of the IPU output(derived independently in the IPU configuratoin SW)
        //     calib_out_ext = FOV.Adapt_Intrinsics(calib_rectified, undist_size_xy, force_fov_symmetry);
        // K_fe = [calib_out_ext.focal_length(1), 0, calib_out_ext.principal_point(1); ...
        //     0, calib_out_ext.focal_length(2), calib_out_ext.principal_point(2); ...
        //     0, 0, 1];
        // % #      A translation from the fisheye distortion output to the Brown distortion input
        //     focal_length_change = calib_sensor.distortion_params(6:7);
        // principal_point_shift = calib_sensor.distortion_params(8:9);
        // K_trans = [1 + focal_length_change(1), 0, principal_point_shift(1); ...
        //     0, 1 + focal_length_change(2), principal_point_shift(2); ...
        //     0, 0, 1];
        // % #      The updated parameters
        //     calib_undist.image_width = calib_out_ext.image_width;
        // calib_undist.image_height = calib_out_ext.image_height;
        // K_brown = K_fe * K_trans;
        // calib_undist.principal_point = K_brown([1, 2], 3)';
        //     calib_undist.focal_length = [K_brown(1, 1), K_brown(2, 2)];
        // % # Distortion: only Brown
        //     calib_undist.distortion_model = 1;
        // calib_undist.distortion_params(6: end) = 0;
        // % # Rotation: unchanged
        void update_table_to_correct_fisheye_distortion(ds::d500_rgb_calibration_table& rgb_calib_table, rs2_intrinsics& intrinsics)
        {
            auto& rgb_coefficients_table = rgb_calib_table.rgb_coefficients_table;

            // checking if the fisheye distortion is needed
            if (rgb_coefficients_table.distortion_model == RS2_DISTORTION_BROWN_CONRADY)
                return;

            // matrix with the intrinsics - after they have been adapted to required resolution
            // k_fe matrix - set as column-major matrix below
            //  {intrinsics.fx,       0      , intrinsics.ppx}
            //  {      0      , intrinsics.fy, intrinsics.ppy}
            //  {      0      ,       0      ,        1      }
            float3x3 k_fe;
            k_fe.x = { intrinsics.fx, 0 ,0 };
            k_fe.y = { 0, intrinsics.fy, 0 };
            k_fe.z = { intrinsics.ppx, intrinsics.ppy, 1 };

            // translation from fisheye distortion output to Brown distortion input
            auto focal_length_change_x = rgb_coefficients_table.distortion_coeffs[5];
            auto focal_length_change_y = rgb_coefficients_table.distortion_coeffs[6];
            auto principal_point_change_x = rgb_coefficients_table.distortion_coeffs[7];
            auto principal_point_change_y = rgb_coefficients_table.distortion_coeffs[8];
            // k_trans matrix - set as column-major matrix below
            // {1 + focal_length_change_x,              0           , principal_point_change_x },
            // {            0            , 1 + focal_length_change_y, principal_point_change_y },
            // {            0            ,              0           ,            1             } };
            float3x3 k_trans;
            k_trans.x = { 1 + focal_length_change_x, 0, 0 };
            k_trans.y = { 0, 1 + focal_length_change_y, 0 };
            k_trans.z = { principal_point_change_x, principal_point_change_y, 1 };

            auto k_brown = k_fe * k_trans;

            intrinsics.ppx = k_brown.z.x;
            intrinsics.ppy = k_brown.z.y;
            intrinsics.fx = k_brown.x.x;
            intrinsics.fy = k_brown.y.y;
            intrinsics.model = RS2_DISTORTION_BROWN_CONRADY;

            // update values in the distortion params of the calibration table
            rgb_coefficients_table.distortion_model = RS2_DISTORTION_BROWN_CONRADY;

            // Since we override the table we need an indication that the table had changed from
            // outside this function Decided to use a reserve field for that
            if( rgb_coefficients_table.reserved[3] != 0 )
                throw invalid_value_exception( "reserved field read from RGB distortion model table is expected to be zero" );

            rgb_coefficients_table.reserved[3] = 1;

            rgb_coefficients_table.distortion_coeffs[5] = 0;
            rgb_coefficients_table.distortion_coeffs[6] = 0;
            rgb_coefficients_table.distortion_coeffs[7] = 0;
            rgb_coefficients_table.distortion_coeffs[8] = 0;
            rgb_coefficients_table.distortion_coeffs[9] = 0;
            rgb_coefficients_table.distortion_coeffs[10] = 0;
            rgb_coefficients_table.distortion_coeffs[11] = 0;
            rgb_coefficients_table.distortion_coeffs[12] = 0;
            // updating crc after values have been modified
            auto ptr = reinterpret_cast<uint8_t*>(&rgb_calib_table);
            std::vector<uint8_t> raw_data(ptr, ptr + sizeof(rgb_calib_table));
            rgb_calib_table.header.crc32 = rsutils::number::calc_crc32(raw_data.data() + sizeof(table_header), raw_data.size() - sizeof(table_header));
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

            // For D555e, model will be brown and we need the unrectified intrinsics
            // We use reserved[3] as a flag here to indicate the full distortion module is not
            // really brown but we treat it as such because the HW fixes fisheye distortion.
            bool use_base_intrinsics
                = ( table->rgb_coefficients_table.distortion_model == RS2_DISTORTION_BROWN_CONRADY
                    && table->rgb_coefficients_table.reserved[3] == 0 );

            auto rect_params = compute_rect_params_from_resolution(
                use_base_intrinsics ? table->rgb_coefficients_table.base_instrinsics
                                    : table->rectified_intrinsics,
                                      width,
                                      height,
                                      false );  // symmetry not needed for RGB

            intrinsics.fx = rect_params[0];
            intrinsics.fy = rect_params[1];
            intrinsics.ppx = rect_params[2];
            intrinsics.ppy = rect_params[3];
            intrinsics.model = table->rgb_coefficients_table.distortion_model;
            std::memcpy( intrinsics.coeffs,
                         table->rgb_coefficients_table.distortion_coeffs,
                         sizeof( intrinsics.coeffs ) );

            update_table_to_correct_fisheye_distortion(const_cast<ds::d500_rgb_calibration_table&>(*table), intrinsics);

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
