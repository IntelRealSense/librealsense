// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include "calibration.h"
#include "../../../include/librealsense2/h/rs_types.h"

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
        float los_scaling_x;
        float los_scaling_y;
        float los_shift_x;
        float los_shift_y;
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
        float EXTLdsmXscale;
        float EXTLdsmYscale;
        float EXTLdsmXoffset;
        float EXTLdsmYoffset;
    };
#pragma pack(pop)

    struct pre_process_data
    {
        los_shift_scaling last_los_error;
        std::vector<double3> vertices_orig;
        std::vector<uint8_t> relevant_pixels_image_rot;
        std::vector<double2> los_orig;

    };

    class k_to_DSM
    {
    public:
        k_to_DSM(const rs2_dsm_params& orig_dsm_params,
            algo::depth_to_rgb_calibration::algo_calibration_info const & cal_info,
            algo::depth_to_rgb_calibration::algo_calibration_registers const & cal_regs);

        algo_calibration_registers apply_ac_res_on_dsm_model(const rs2_dsm_params& ac_data, const algo_calibration_registers& regs, const ac_to_dsm_dir& type);

        los_shift_scaling convert_ac_data_to_los_error(const algo_calibration_registers& algo_calibration_registers, const rs2_dsm_params& ac_data);

        pre_process_data pre_processing(const algo_calibration_info& regs,
            const rs2_dsm_params& ac_data,
            const algo_calibration_registers& algo_calibration_registers,
            const rs2_intrinsics_double& k_raw,
            const std::vector<uint8_t>& relevant_pixels_image);

        rs2_dsm_params convert_new_k_to_DSM(const rs2_intrinsics_double& old_k,
            const rs2_intrinsics_double& new_k,
            const std::vector<uint8_t>& relevant_pixels_image);

        const pre_process_data& get_pre_process_data() const;

    private:
        std::vector<double3> calc_relevant_vertices(const std::vector<uint8_t>& relevant_pixels_image, 
            const rs2_intrinsics_double& k);

        std::vector<double2> convert_norm_vertices_to_los(const algo_calibration_info& regs, 
            const algo_calibration_registers& algo_calibration_registers, 
            std::vector<double3> vertices);

        std::vector<double3> transform_to_direction(std::vector<double3>);
        
        pre_process_data _pre_process_data;

        rs2_dsm_params _orig_dsm_params;
        algo::depth_to_rgb_calibration::algo_calibration_info _cal_info;
        algo::depth_to_rgb_calibration::algo_calibration_registers _cal_regs;
    };
   


}
}
}

