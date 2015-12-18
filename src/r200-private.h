/*
    INTEL CORPORATION PROPRIETARY INFORMATION This software is supplied under the
    terms of a license agreement or nondisclosure agreement with Intel Corporation
    and may not be copied or disclosed except in accordance with the terms of that
    agreement.
    Copyright(c) 2015 Intel Corporation. All Rights Reserved.
*/

#pragma once
#ifndef LIBREALSENSE_R200_PRIVATE_H
#define LIBREALSENSE_R200_PRIVATE_H

#include "uvc.h"

#include <cstring>

namespace rsimpl 
{
    namespace r200
    {       
        const int STATUS_BIT_Z_STREAMING = 1 << 0;
        const int STATUS_BIT_LR_STREAMING = 1 << 1;
        const int STATUS_BIT_WEB_STREAMING = 1 << 2;

        #pragma pack(push, 1)
        const int CURRENT_CAMERA_CONTENTS_VERSION_NUMBER = 12;

        struct CameraHeaderInfo
        {
            uint32_t serialNumber;
            uint32_t modelNumber;
            uint32_t revisionNumber;
            uint8_t modelData[64];
            double buildDate;
            double firstProgramDate;
            double focusAndAlignmentDate;
            uint32_t nominalBaselineThird;
            uint8_t moduleVersion;
            uint8_t moduleMajorVersion;
            uint8_t moduleMinorVersion;
            uint8_t moduleSkew;
            uint32_t lensTypeThird;
            uint32_t OEMID;
            uint32_t lensCoatingTypeThird;
            uint8_t platformCameraSupport;
            uint8_t reserved1[3];
            uint32_t emitterType;
            uint8_t reserved2[4];
            uint32_t cameraFPGAVersion;
            uint32_t platformCameraFocus; // This is the value during calibration
            double calibrationDate;
            uint32_t calibrationType;
            double calibrationXError;
            double calibrationYError;
            double rectificationDataQres[54];
            double rectificationDataPadding[26];
            double CxQres;
            double CyQres;
            double CzQres;
            double KxQres;
            double KyQres;
            uint32_t cameraHeadContentsVersion;
            uint32_t cameraHeadContentsSizeInBytes;
            double CxBig;
            double CyBig;
            double CzBig;
            double KxBig;
            double KyBig;
            double CxSpecial;
            double CySpecial;
            double CzSpecial;
            double KxSpecial;
            double KySpecial;
            uint8_t cameraHeadDataLittleEndian;
            double rectificationDataBig[54];
            double rectificationDataSpecial[54];
            uint8_t cameraOptions1;
            uint8_t cameraOptions2;
            uint8_t bodySerialNumber[20];
            double Dx;
            double Dy;
            double Dz;
            double ThetaX;
            double ThetaY;
            double ThetaZ;
            double registrationDate;
            double registrationRotation[9];
            double registrationTranslation[3];
            uint32_t nominalBaseline;
            uint32_t lensType;
            uint32_t lensCoating;
            int32_t nominalBaselinePlatform[3]; // NOTE: Signed, since platform camera can be mounted anywhere
            uint32_t lensTypePlatform;
            uint32_t imagerTypePlatform;
            uint32_t theLastWord;
            uint8_t reserved3[37];
        };

        struct Dinghy
        {
            uint32_t magicNumber;
            uint32_t frameCount;
            uint32_t frameStatus;
            uint32_t exposureLeftSum;
            uint32_t exposureLeftDarkCount;
            uint32_t exposureLeftBrightCount;
            uint32_t exposureRightSum;
            uint32_t exposureRightDarkCount;
            uint32_t exposureRightBrightCount;
            uint32_t CAMmoduleStatus;
            uint32_t pad0;
            uint32_t pad1;
            uint32_t pad2;
            uint32_t pad3;
            uint32_t VDFerrorStatus;
            uint32_t pad4;
        };

        struct auto_exposure_params
        {
            float mean_intensity;
            float bright_ratio;
            float kp_gain;
            float kp_exposure;
            float kp_dark_threshold;
            uint16_t region_of_interest_top_left;
            uint16_t region_of_interest_top_right;
            uint16_t region_of_interest_bottom_left;
            uint16_t region_of_interest_bottom_right;
        };

        struct depth_params
        {
            uint32_t robbins_munroe_minus_inc;
            uint32_t robbins_munroe_plus_inc;
            uint32_t median_thresh;
            uint32_t score_min_thresh;
            uint32_t score_max_thresh;
            uint32_t texture_count_thresh;
            uint32_t texture_diff_thresh;
            uint32_t second_peak_thresh;
            uint32_t neighbor_thresh;
            uint32_t lr_thresh;

            enum { MAX_PRESETS = 6 };
            static const depth_params presets[MAX_PRESETS];
        };

        struct disparity_mode
        {
            uint32_t is_disparity_enabled;
            double disparity_multiplier;
        };
        #pragma pack(pop)

        struct r200_calibration
        {
            int version;
            rs_intrinsics modesLR[3];
            rs_intrinsics intrinsicsThird[2];
            rs_intrinsics modesThird[2][2];
            float Rthird[9], T[3], B;
        };

        std::string read_firmware_version(uvc::device & device);
        void read_camera_info(uvc::device & device, r200_calibration & calib, CameraHeaderInfo & header);
             
        //void xu_read(const uvc::device & device, uint8_t xu_ctrl, void * buffer, uint32_t length);
        //void xu_write(uvc::device & device, uint8_t xu_ctrl, void * buffer, uint32_t length);
             
        void set_stream_intent(uvc::device & device, uint8_t & intent);
        void get_stream_status(const uvc::device & device, uint8_t & status);
             
        void get_last_error(const uvc::device & device, uint8_t & last_error);
        void force_firmware_reset(uvc::device & device);
             
        void get_emitter_state(const uvc::device & device, bool is_streaming, bool is_depth_enabled, bool & state);
        void set_emitter_state(uvc::device & device, bool state);
             
        void read_temperature(const uvc::device & device, int8_t & current, int8_t & min, int8_t & max, int8_t & min_fault);
        void reset_temperature(uvc::device & device);
             
        void get_depth_units(const uvc::device & device, uint32_t & units);
        void set_depth_units(uvc::device & device, uint32_t units);
             
        void get_min_max_depth(const uvc::device & device, uint16_t & min_depth, uint16_t & max_depth);
        void set_min_max_depth(uvc::device & device, uint16_t min_depth, uint16_t max_depth);
             
        void get_lr_gain(const uvc::device & device, uint32_t & rate, uint32_t & gain);
        void set_lr_gain(uvc::device & device, uint32_t rate, uint32_t gain);
             
        void get_lr_exposure(const uvc::device & device, uint32_t & rate, uint32_t & exposure);
        void set_lr_exposure(uvc::device & device, uint32_t rate, uint32_t exposure);
             
        void get_lr_auto_exposure_params(const uvc::device & device, auto_exposure_params & params);
        void set_lr_auto_exposure_params(uvc::device & device, auto_exposure_params params);
             
        void get_lr_exposure_mode(const uvc::device & device, uint32_t & mode);
        void set_lr_exposure_mode(uvc::device & device, uint32_t mode);
             
        void get_depth_params(const uvc::device & device, depth_params & params);
        void set_depth_params(uvc::device & device, depth_params params);
             
        void get_disparity_mode(const uvc::device & device, disparity_mode & mode);
        void set_disparity_mode(uvc::device & device, disparity_mode mode);
             
        void get_disparity_shift(const uvc::device & device, uint32_t & shift);
        void set_disparity_shift(uvc::device & device, uint32_t shift);

		void get_register_value(uvc::device & device, uint32_t reg, uint32_t & value);
		void set_register_value(uvc::device & device, uint32_t reg, uint32_t value);
        
        // todo - (if necessary) - get_exposure_discovery
        // todo - (if necessary) - set_exposure_discovery
        // todo - (if necessary) - get_gain_discovery
        // todo - (if necessary) - set_gain_discovery
    }
}

#endif // R200PRIVATE_H
