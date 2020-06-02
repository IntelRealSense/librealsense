// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include "calibration.h"
#include "frame-data.h"
#include <librealsense2/h/rs_types.h>

#include <types.h>  // librealsense types (intr/extr)

namespace librealsense {
namespace algo {
namespace depth_to_rgb_calibration {

    enum ac_to_dsm_dir
    {
        direct,
        inverse
    };

    enum dsm_model
    {
        none = 0,
        AOT = 1,
        TOA = 2
    };


    struct los_shift_scaling
    {
        double los_scaling_x;
        double los_scaling_y;
        double los_shift_x;
        double los_shift_y;
    };

    struct rs2_dsm_params_double
    {
        rs2_dsm_params_double(rs2_dsm_params obj)
            :timestamp(obj.timestamp),
            version(obj.version),
            model(obj.model),
            h_scale(obj.h_scale),
            v_scale(obj.v_scale),
            h_offset(obj.h_offset),
            v_offset(obj.v_offset),
            rtd_offset(obj.rtd_offset),
            temp_x2(obj.temp_x2){} //todo: flags

        rs2_dsm_params_double() = default;

        operator rs2_dsm_params()
        {
            rs2_dsm_params res;

            res.timestamp = timestamp;
            res.version = version;
            res.model = model;
            for (auto i = 0; i < 5; i++)
                res.flags[i] = flags[i];

            res.h_scale = float(h_scale);
            res.v_scale = float(v_scale);
            res.h_offset = float(h_offset);
            res.v_offset = float(v_offset);
            res.rtd_offset = float(rtd_offset);
            res.temp_x2 = temp_x2;
            return res;
        }

        unsigned long long timestamp;   /**< system_clock::time_point::time_since_epoch().count() */
        unsigned short version;         /**< MAJOR<<12 | MINOR<<4 | PATCH */
        unsigned char model;            /**< rs2_dsm_correction_model */
        unsigned char flags[5];         /**< TBD, now 0s */
        double        h_scale;          /**< the scale factor to horizontal DSM scale thermal results */
        double        v_scale;          /**< the scale factor to vertical DSM scale thermal results */
        double        h_offset;         /**< the offset to horizontal DSM offset thermal results */
        double        v_offset;         /**< the offset to vertical DSM offset thermal results */
        double        rtd_offset;       /**< the offset to the Round-Trip-Distance delay thermal results */
        unsigned char temp_x2;          /**< the temperature recorded times 2 (ldd for depth; hum for rgb) */
        unsigned char reserved[11];
    };
    
#pragma pack(push, 1)
    // This table is read from FW and is used in the optimizer
    struct algo_calibration_info
    {
        static const int table_id = 0x313;

        uint16_t version;       // = 0x0100
        uint16_t id;            // = table_id
        uint32_t size;          // = 0x1F0
        uint32_t full_version;  // = 0xFFFFFFFF
        uint32_t crc32;         // of the following data:
        uint32_t DIGGundistFx;
        uint32_t DIGGundistFy;
        int32_t  DIGGundistX0;
        int32_t  DIGGundistY0;
        uint8_t  DESThbaseline;
        float    DESTbaseline;
        float    FRMWxfov[5];
        float    FRMWyfov[5];
        float    FRMWprojectionYshear[5];
        float    FRMWlaserangleH;
        float    FRMWlaserangleV;
        uint16_t FRMWcalMarginL;
        uint16_t FRMWcalMarginR;
        uint16_t FRMWcalMarginT;
        uint16_t FRMWcalMarginB;
        uint8_t  FRMWxR2L;
        uint8_t  FRMWyflip;
        float    EXTLdsmXscale;
        float    EXTLdsmYscale;
        float    EXTLdsmXoffset;
        float    EXTLdsmYoffset;
        uint32_t EXTLconLocDelaySlow;
        uint32_t EXTLconLocDelayFastC;
        uint32_t EXTLconLocDelayFastF;
        uint16_t FRMWcalImgHsize;
        uint16_t FRMWcalImgVsize;
        float    FRMWpolyVars[3];
        float    FRMWpitchFixFactor;
        uint32_t FRMWzoRawCol[5];
        uint32_t FRMWzoRawRow[5];
        float    FRMWdfzCalTmp;
        float    FRMWdfzVbias[3];
        float    FRMWdfzIbias[3];
        float    FRMWdfzApdCalTmp;
        float    FRMWatlMinVbias1;
        float    FRMWatlMaxVbias1;
        float    FRMWatlMinVbias2;
        float    FRMWatlMaxVbias2;
        float    FRMWatlMinVbias3;
        float    FRMWatlMaxVbias3;
        float    FRMWundistAngHorz[4];
        float    FRMWundistAngVert[4];
        uint8_t  FRMWfovexExistenceFlag;
        float    FRMWfovexNominal[4];
        uint8_t  FRMWfovexLensDistFlag;
        float    FRMWfovexRadialK[3];
        float    FRMWfovexTangentP[2];
        float    FRMWfovexCenter[2];
        uint32_t FRMWcalibVersion;
        uint32_t FRMWconfigVersion;
        uint32_t FRMWeepromVersion;
        float    FRMWconLocDelaySlowSlope;
        float    FRMWconLocDelayFastSlope;
        int16_t  FRMWatlMinAngXL;
        int16_t  FRMWatlMinAngXR;
        int16_t  FRMWatlMaxAngXL;
        int16_t  FRMWatlMaxAngXR;
        int16_t  FRMWatlMinAngYU;
        int16_t  FRMWatlMinAngYB;
        int16_t  FRMWatlMaxAngYU;
        int16_t  FRMWatlMaxAngYB;
        float    FRMWatlSlopeMA;
        float    FRMWatlMaCalTmp;
        uint8_t  FRMWvddVoltValues[3];  // this one is really 2 uint12 values, or 24 bits total; not in use!
        int16_t  FRMWvdd2RtdDiff;
        int16_t  FRMWvdd3RtdDiff;
        int16_t  FRMWvdd4RtdDiff;
        float    FRMWdfzCalibrationLddTemp;
        float    FRMWdfzCalibrationVddVal;
        float    FRMWhumidApdTempDiff;
        uint8_t  reserved[94];
    };

    struct algo_calibration_registers
    {
        double EXTLdsmXscale;
        double EXTLdsmYscale;
        double EXTLdsmXoffset;
        double EXTLdsmYoffset;
    };
#pragma pack(pop)

    struct pre_process_data
    {
        rs2_intrinsics_double orig_k;
        los_shift_scaling last_los_error;
        std::vector<double3> vertices_orig;
        std::vector<uint8_t> relevant_pixels_image_rot;
        std::vector<double2> los_orig;

    };
    struct iteration_data_collect;

    class k_to_DSM
    {
    public:
        k_to_DSM(const rs2_dsm_params_double& orig_dsm_params,
            algo::depth_to_rgb_calibration::algo_calibration_info const & cal_info,
            algo::depth_to_rgb_calibration::algo_calibration_registers const & cal_regs,
            const double& max_scaling_step);

        algo_calibration_registers apply_ac_res_on_dsm_model(const rs2_dsm_params_double& ac_data,
            const algo_calibration_registers& regs, 
            const ac_to_dsm_dir& type);

        los_shift_scaling convert_ac_data_to_los_error(const algo_calibration_registers& algo_calibration_registers, 
            const rs2_dsm_params_double& ac_data);

        pre_process_data pre_processing(const algo_calibration_info& regs,
            const rs2_dsm_params_double& ac_data,
            const algo_calibration_registers& algo_calibration_registers,
            const rs2_intrinsics_double& k_raw,
            const std::vector<uint8_t>& relevant_pixels_image);

        //return rs2_dsm_params and new vertices
        rs2_dsm_params_double convert_new_k_to_DSM(
            const rs2_intrinsics_double& old_k,
            const rs2_intrinsics_double& new_k,
            const z_frame_data& z,
            std::vector<double3>& new_vertices,
            iteration_data_collect* data = nullptr);

        const pre_process_data& get_pre_process_data() const;

    private:
        double2 convert_k_to_los_error(
            algo::depth_to_rgb_calibration::algo_calibration_info const & regs,
            algo_calibration_registers const &dsm_regs,
            rs2_intrinsics_double const & k_raw);

        rs2_dsm_params_double convert_los_error_to_ac_data(
            const rs2_dsm_params_double& ac_data,
            const algo_calibration_registers& dsm_regs,
            double2 los_shift, double2 los_scaling);

        double2 run_scaling_optimization_step(
            algo::depth_to_rgb_calibration::algo_calibration_info const & regs,
            algo_calibration_registers const &dsm_regs,
            double scaling_grid_x[25],
            double scaling_grid_y[25],
            double2 focal_scaling);

        std::vector<double3x3> optimize_k_under_los_error(
            algo::depth_to_rgb_calibration::algo_calibration_info const & regs,
            algo_calibration_registers const &dsm_regs, 
            double scaling_grid_x[25],
            double scaling_grid_y[25] );

        std::vector<double3> convert_los_to_norm_vertices(
            algo::depth_to_rgb_calibration::algo_calibration_info const & regs,
            algo_calibration_registers const &dsm_regs,
            std::vector<double2> los,
            iteration_data_collect* data = nullptr);

        std::vector<double3> calc_relevant_vertices(const std::vector<uint8_t>& relevant_pixels_image, 
            const rs2_intrinsics_double& k);

        std::vector<double2> convert_norm_vertices_to_los(const algo_calibration_info& regs, 
            const algo_calibration_registers& algo_calibration_registers, 
            std::vector<double3> vertices);

        double3 laser_incident_direction(double2 angle_rad);
        std::vector<double3> transform_to_direction(std::vector<double3>);
        
        pre_process_data _pre_process_data;

        //debug data
        double2 _new_los_scaling;

        //input camera params
        rs2_dsm_params_double _ac_data;
        algo::depth_to_rgb_calibration::algo_calibration_info _regs;
        algo::depth_to_rgb_calibration::algo_calibration_registers _dsm_regs;

        //algorithem params
        double _max_scaling_step;
    };
   


}
}
}

