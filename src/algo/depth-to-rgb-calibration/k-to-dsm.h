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

    static const int SIZE_OF_GRID_X = 25;
    static const int SIZE_OF_GRID_Y = 6;

    enum ac_to_dsm_dir
    {
        direct,
        inverse
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
        rs2_dsm_correction_model model;
        double h_scale;    /**< the scale factor to horizontal DSM scale thermal results */
        double v_scale;    /**< the scale factor to vertical DSM scale thermal results */
        double h_offset;   /**< the offset to horizontal DSM offset thermal results */
        double v_offset;   /**< the offset to vertical DSM offset thermal results */
        double rtd_offset; /**< the offset to the Round-Trip-Distance delay thermal results */

        rs2_dsm_params_double() = default;
        rs2_dsm_params_double( rs2_dsm_params const & obj )
            : model( rs2_dsm_correction_model( obj.model ) )
            , h_scale( obj.h_scale )
            , v_scale( obj.v_scale )
            , h_offset( obj.h_offset )
            , v_offset( obj.v_offset )
            , rtd_offset( obj.rtd_offset )
        {
            //todo: flags
        }

        void copy_to( rs2_dsm_params & o )
        {
            o.h_scale = float( h_scale );
            o.v_scale = float( v_scale );
            o.h_offset = float( h_offset );
            o.v_offset = float( v_offset );
            o.rtd_offset = float( rtd_offset );
            o.model = model;
        }
    };

    std::ostream & operator<<( std::ostream &, rs2_dsm_params_double const & );

#pragma pack(push, 1)
    // This table is read from FW and is used in the optimizer
    // Best way to see this table and its formatting is in the Algo source code, under:
    //     eepromStructure/eepromStructure.mat
    struct algo_calibration_info
    {
        static const int table_id = 0x313;

        uint32_t DIGGundistFx;  // x frequency of undist lut (1/distance between to neighbors in lut)
        uint32_t DIGGundistFy;  // y frequency of undist lut (1/distance between to neighbors in lut)
        int32_t  DIGGundistX0;  // x input added offset- luts start from 0-0 but we can have negative x/y input
        int32_t  DIGGundistY0;  // y input added offset- luts start from 0-0 but we can have negative x/y input
        uint8_t  DESThbaseline;  // 0:/5;1:/95
        float    DESTbaseline;  // [-35.0:35.0]:/50;[-160.0:160.0]:/45;[100.0:1000.0]:/5
        float    DESTtxFRQpd[3];     //!!
        float    FRMWxfov[5];  // output projection x fov [7.0:100.0]:/50;72.0:/50
        float    FRMWyfov[5];  // output projection y fov [7.0:90.0]:/50;56.0:/50
        float    FRMWprojectionYshear[5];  // output projection y shearing [-0.03:0.03]
        float    FRMWlaserangleH;  // laser-mems angle [-3.0:3.0]
        float    FRMWlaserangleV;
        int16_t  FRMWcalMarginL;  // image margin [-256:1280]
        int16_t  FRMWcalMarginR;
        int16_t  FRMWcalMarginT;
        int16_t  FRMWcalMarginB;
        uint8_t  FRMWxR2L;  // case the input angle arrives from high to low
        uint8_t  FRMWyflip;  // flip the sign of the fast scan
        float    EXTLdsmXscale;  // DSM conversion parameter [0.0:100000.0]
        float    EXTLdsmYscale;
        float    EXTLdsmXoffset;
        float    EXTLdsmYoffset;
        uint32_t EXTLconLocDelaySlow;  // RegsAnsyncAsLateLatencyFixEn
        uint32_t EXTLconLocDelayFastC;  // RegsProjConLocDelay
        uint32_t EXTLconLocDelayFastF;  // RegsProjConLocDelayHfclkRes
        uint16_t FRMWcalImgHsize;  // horizontal image size in pixels
        uint16_t FRMWcalImgVsize;  // vertical image size in pixels
        float    FRMWpolyVars[3];  // horizontal angle stage 1 undist x^1 coefficient
        float    FRMWpitchFixFactor;  // vertical angle stage 1 undist x^1 coefficient
        uint32_t FRMWzoRawCol[5];  // Cal resolution zero order raw -x location
        uint32_t FRMWzoRawRow[5];  // Cal resolution zero order raw -y location
        float    FRMWdfzCalTmp;  // DFZ Calibration temperature [-1.0:90.0]
        float    FRMWdfzVbias[3];  // DFZ Calibration vBias1 [0.0:90.0]
        float    FRMWdfzIbias[3];  // DFZ Calibration iBias1 [0.0:90.0]
        float    FRMWdfzApdCalTmp;  // DFZ Apd Calibration temperature [-1.0:90.0]
        float    FRMWatlMinVbias1;  // Algo Thermal Loop Calibration min vBias1 [0.0:90.0]
        float    FRMWatlMaxVbias1;  // Algo Thermal Loop Calibration max vBias1 [0.0:90.0]
        float    FRMWatlMinVbias2;  // Algo Thermal Loop Calibration min vBias2 [0.0:90.0]
        float    FRMWatlMaxVbias2;  // Algo Thermal Loop Calibration max vBias2 [0.0:90.0]
        float    FRMWatlMinVbias3;  // Algo Thermal Loop Calibration min vBias3 [0.0:90.0]
        float    FRMWatlMaxVbias3;  // Algo Thermal Loop Calibration max vBias3 [0.0:90.0]
        float    FRMWundistAngHorz[4];  // horz angle stage 2 undist: x^# coefficient
        float    FRMWundistAngVert[4];  // vert angle stage 2 undist: y^# coefficient
        uint8_t  FRMWfovexExistenceFlag;  // fovex existence flag (0 = no FOVex)
        float    FRMWfovexNominal[4];  // fovex nominal opening model: r^# coefficient
        uint8_t  FRMWfovexLensDistFlag;  // fovex lens distortion flag (1 = apply lens distortion model)
        float    FRMWfovexRadialK[3];  // fovex radial distortion: r^# coefficient 
        float    FRMWfovexTangentP[2];  // fovex tangential distortion: r^2+2x^2 coefficient
        float    FRMWfovexCenter[2];  // fovex distortion center (horz & vert)
        uint32_t FRMWcalibVersion;
        uint32_t FRMWconfigVersion;
        uint32_t FRMWeepromVersion;  // EEPROM structure version
        float    FRMWconLocDelaySlowSlope;  // slope of Z-IR delay correction vs. LDD temperature [nsec/deg]
        float    FRMWconLocDelayFastSlope;  // slope of Z delay correction vs. LDD temperature [nsec/deg]
        int16_t  FRMWatlMinAngXL;  // min DSM X angle on the lower side
        int16_t  FRMWatlMinAngXR;  // min DSM X angle on the higher side
        int16_t  FRMWatlMaxAngXL;  // max DSM X angle on the lower side
        int16_t  FRMWatlMaxAngXR;  // max DSM X angle on the higher side
        int16_t  FRMWatlMinAngYU;  // min DSM Y angle on the lower side
        int16_t  FRMWatlMinAngYB;  // min DSM Y angle on the higher side
        int16_t  FRMWatlMaxAngYU;  // max DSM Y angle on the lower side
        int16_t  FRMWatlMaxAngYB;  // max DSM Y angle on the higher side
        float    FRMWatlSlopeMA;  // Thermal rtd fix slope - deltaRtd = ma*slope + offset
        float    FRMWatlMaCalTmp;  // Final ATC MA Calibration temperature
        uint8_t  FRMWvddVoltValues[3];  // sampled vdd voltage as measured during ACC calibration (2 uint12 values, or 24 bits total)
        int16_t  FRMWvdd2RtdDiff;  // Range difference between 2nd vdd voltage and the 1st vdd voltage
        int16_t  FRMWvdd3RtdDiff;
        int16_t  FRMWvdd4RtdDiff;
        float    FRMWdfzCalibrationLddTemp;  // ldd temperature during DFZ calibration at the ACC
        float    FRMWdfzCalibrationVddVal;  // vdd value as measured during DFZ calibration at the ACC
        float    FRMWhumidApdTempDiff;  // difference between SHTW2 and TSense readings at first TSense temperature above 20deg
        float    FRMWlosAtMirrorRestHorz;  // horizontal LOS report at mirror rest during DSM calibration
        float    FRMWlosAtMirrorRestVert;  // vertical LOS report at mirror rest during DSM calibration
        uint8_t  reserved[90];
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
    struct data_collect;
    struct convert_norm_vertices_to_los_data;

    class k_to_DSM
    {
    public:
        k_to_DSM(const rs2_dsm_params_double& orig_dsm_params,
            algo_calibration_info const & cal_info,
            algo_calibration_registers const & cal_regs,
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
            const std::vector<uint8_t>& relevant_pixels_image,
            data_collect* data = nullptr);

        //return rs2_dsm_params and new vertices
        rs2_dsm_params_double convert_new_k_to_DSM(
            const rs2_intrinsics_double& old_k,
            const rs2_intrinsics_double& new_k,
            const z_frame_data& z,
            std::vector<double3>& new_vertices,
            rs2_dsm_params_double const & previous_dsm_params,
            algo_calibration_registers& new_dsm_regs,
            data_collect* data = nullptr);

        const pre_process_data& get_pre_process_data() const;

        algo_calibration_info const & get_calibration_info() const { return _regs; }
        algo_calibration_registers const & get_calibration_registers() const { return _dsm_regs; }

    private:
        double2 convert_k_to_los_error(
            algo::depth_to_rgb_calibration::algo_calibration_info const & regs,
            algo_calibration_registers const &dsm_regs,
            rs2_intrinsics_double const & k_raw,
            data_collect* data = nullptr);

        rs2_dsm_params_double convert_los_error_to_ac_data(
            const rs2_dsm_params_double& ac_data,
            const algo_calibration_registers& dsm_regs,
            double2 los_shift, double2 los_scaling);

        double2 run_scaling_optimization_step(
            algo::depth_to_rgb_calibration::algo_calibration_info const & regs,
            algo_calibration_registers const &dsm_regs,
            double scaling_grid_x[SIZE_OF_GRID_X],
            double scaling_grid_y[SIZE_OF_GRID_X],
            double2 focal_scaling,
            data_collect* data = nullptr);

        std::vector<double3x3> optimize_k_under_los_error(
            algo::depth_to_rgb_calibration::algo_calibration_info const & regs,
            algo_calibration_registers const &dsm_regs, 
            double scaling_grid_x[25],
            double scaling_grid_y[25] );

        std::vector<double3> convert_los_to_norm_vertices(
            algo::depth_to_rgb_calibration::algo_calibration_info const & regs,
            algo_calibration_registers const &dsm_regs,
            std::vector<double2> los,
            data_collect* data = nullptr);

        std::vector<double3> calc_relevant_vertices(const std::vector<uint8_t>& relevant_pixels_image, 
            const rs2_intrinsics_double& k);

        std::vector<double2> convert_norm_vertices_to_los(const algo_calibration_info& regs, 
            const algo_calibration_registers& algo_calibration_registers, 
            std::vector<double3> const & vertices, 
            convert_norm_vertices_to_los_data* data = nullptr);

        double3 laser_incident_direction(double2 angle_rad);
        std::vector< double3 > transform_to_direction( std::vector< double3 > const & );
        
        pre_process_data _pre_process_data;

        //input camera params
        algo::depth_to_rgb_calibration::algo_calibration_info _regs;
        algo::depth_to_rgb_calibration::algo_calibration_registers _dsm_regs;

        //algorithem params
        double _max_scaling_step;
    };
   


}
}
}


