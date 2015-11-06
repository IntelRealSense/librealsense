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
        struct CalibrationMetadata
        {
            big_endian<uint32_t> versionNumber;
            big_endian<uint16_t> numIntrinsicsRight;
            big_endian<uint16_t> numIntrinsicsThird;
            big_endian<uint16_t> numIntrinsicsPlatform;
            big_endian<uint16_t> numRectifiedModesLR;
            big_endian<uint16_t> numRectifiedModesThird;
            big_endian<uint16_t> numRectifiedModesPlatform;
        };

        struct UnrectifiedIntrinsics
        {
            big_endian<float> fx;
            big_endian<float> fy;
            big_endian<float> px;
            big_endian<float> py;
            big_endian<float> k[5];
            big_endian<uint32_t> w;
            big_endian<uint32_t> h;
        };

        struct RectifiedIntrinsics
        {
            big_endian<float> rfx;
            big_endian<float> rfy;
            big_endian<float> rpx;
            big_endian<float> rpy;
            big_endian<uint32_t> rw;
            big_endian<uint32_t> rh;
        };

        struct CameraCalibrationParameters
        {
            static const int MAX_NUM_INTRINSICS_RIGHT = 2; // Max number right cameras supported (e.g. one or two, two would support a multi-baseline unit)
            static const int MAX_NUM_INTRINSICS_THIRD = 3; // Max number native resolutions the third camera can have (e.g. 1920x1080 and 640x480)
            static const int MAX_NUM_INTRINSICS_PLATFORM = 4; // Max number native resolutions the platform camera can have
            static const int MAX_NUM_RECTIFIED_MODES_LR = 4; // Max number rectified LR resolution modes the structure supports (e.g. 640x480, 492x372 and 332x252)
            static const int MAX_NUM_RECTIFIED_MODES_THIRD = 3; // Max number rectified Third resolution modes the structure supports (e.g. 1920x1080, 1280x720, etc)
            static const int MAX_NUM_RECTIFIED_MODES_PLATFORM = 1; // Max number rectified Platform resolution modes the structure supports

            CalibrationMetadata metadata;

            UnrectifiedIntrinsics intrinsicsLeft;
            UnrectifiedIntrinsics intrinsicsRight[MAX_NUM_INTRINSICS_RIGHT];
            UnrectifiedIntrinsics intrinsicsThird[MAX_NUM_INTRINSICS_THIRD];
            UnrectifiedIntrinsics intrinsicsPlatform[MAX_NUM_INTRINSICS_PLATFORM];

            RectifiedIntrinsics modesLR[MAX_NUM_INTRINSICS_RIGHT * MAX_NUM_RECTIFIED_MODES_LR];
            RectifiedIntrinsics modesThird[MAX_NUM_INTRINSICS_RIGHT * MAX_NUM_INTRINSICS_THIRD * MAX_NUM_RECTIFIED_MODES_THIRD];
            RectifiedIntrinsics modesPlatform[MAX_NUM_INTRINSICS_RIGHT * MAX_NUM_INTRINSICS_PLATFORM * MAX_NUM_RECTIFIED_MODES_PLATFORM];

            big_endian<float> Rleft[MAX_NUM_INTRINSICS_RIGHT][9];
            big_endian<float> Rright[MAX_NUM_INTRINSICS_RIGHT][9];
            big_endian<float> Rthird[MAX_NUM_INTRINSICS_RIGHT][9];
            big_endian<float> Rplatform[MAX_NUM_INTRINSICS_RIGHT][9];

            big_endian<float> B[MAX_NUM_INTRINSICS_RIGHT];
            big_endian<float> T[MAX_NUM_INTRINSICS_RIGHT][3];
            big_endian<float> Tplatform[MAX_NUM_INTRINSICS_RIGHT][3];

            big_endian<float> Rworld[9];
            big_endian<float> Tworld[3];
        };

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

        enum class range_format : uint32_t
        {
            RANGE_FORMAT_DISPARITY,
            RANGE_FORMAT_DISTANCE
        };

        struct disparity_mode
        {
            range_format format;
            uint64_t multiplier;
        };

        struct CommandPacket
        {
            uint32_t code;
            uint32_t modifier;
            uint32_t tag;
            uint32_t address;
            uint32_t value;
            uint32_t reserved[59];

            CommandPacket(uint32_t code = 0, uint32_t modifier = 0, uint32_t tag = 0, uint32_t address = 0, uint32_t value = 0)
                : code(code), modifier(modifier), tag(tag), address(address), value(value)
            {
                std::memset(reserved, 0, sizeof(reserved));
            }

        };

        struct ResponsePacket
        {
            uint32_t code;
            uint32_t modifier;
            uint32_t tag;
            uint32_t responseCode;
            uint32_t value;
            uint32_t revision[4];
            uint32_t reserved[55];

            ResponsePacket(uint32_t code = 0, uint32_t modifier = 0, uint32_t tag = 0, uint32_t responseCode = 0, uint32_t value = 0)
                : code(code), modifier(modifier), tag(tag), responseCode(responseCode), value(value)
            {
                std::memset(revision, 0, sizeof(revision));
                std::memset(reserved, 0, sizeof(reserved));
            }
        };
        #pragma pack(pop)

        void send_command(uvc::device & device, CommandPacket & command, ResponsePacket & response);

        std::string read_firmware_version(uvc::device & device);
        void read_camera_info(uvc::device & device, CameraCalibrationParameters & calib, CameraHeaderInfo & header);
             
        void xu_read(const uvc::device & device, uint8_t xu_ctrl, void * buffer, uint32_t length);
        void xu_write(uvc::device & device, uint8_t xu_ctrl, void * buffer, uint32_t length);
             
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
        
        //@todo - (if necessary) - get_exposure_discovery
        //@todo - (if necessary) - set_exposure_discovery
        //@todo - (if necessary) - get_gain_discovery
        //@todo - (if necessary) - set_gain_discovery
    }
}

#endif // R200PRIVATE_H
