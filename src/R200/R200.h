#pragma once

#ifndef LIBREALSENSE_R200_CAMERA_H
#define LIBREALSENSE_R200_CAMERA_H

#include "../UVCCamera.h"

#ifdef USE_UVC_DEVICES
namespace r200
{
    class R200Camera : public rs::UVCCamera
    {
    public:
        R200Camera(uvc_context_t * ctx, uvc_device_t * device);

        void EnableStreamPreset(int streamIdentifier, int preset) override final;

        int GetStreamSubdeviceNumber(int stream) const override final;
        rs::CalibrationInfo RetrieveCalibration(uvc_device_handle_t * handle) override final;
        void SetStreamIntent() override final;
    };

    struct CameraCalibrationParameters;
    struct CameraHeaderInfo;

    // Hardware API for R200 camera
    std::string read_firmware_version(uvc_device_handle_t * device);
    void        read_camera_info(uvc_device_handle_t * device, CameraCalibrationParameters & calib, CameraHeaderInfo & header);
    int         read_stream_status(uvc_device_handle_t * device);

    bool        xu_read(uvc_device_handle_t * device, uint64_t xu_ctrl, uint8_t * buffer, uint32_t length);
    bool        xu_write(uvc_device_handle_t * device, uint64_t xu_ctrl, uint8_t * buffer, uint32_t length);
    bool        get_emitter_state(uvc_device_handle_t * device, bool & state);
    bool        set_emitter_state(uvc_device_handle_t * device, bool state);
    bool        read_temperature(uvc_device_handle_t * device, int8_t & current, int8_t & min, int8_t & max, int8_t & min_fault);
    bool        reset_temperature(uvc_device_handle_t * device);
    bool        get_last_error(uvc_device_handle_t * device, uint8_t & last_error);
    // TODO: Gather additional hardware specific APIs here

    // Implementation details

    #pragma pack(push, 1)
    template<class T> class big_endian
    {
        T be_value;
    public:
        operator T () const
        {
            T le_value;
            for(int i=0; i<sizeof(T); ++i) reinterpret_cast<char *>(&le_value)[i] = reinterpret_cast<const char *>(&be_value)[sizeof(T)-i-1];
            return le_value;
        }
    };

    const uint16_t MAX_NUM_INTRINSICS_RIGHT = 2; // Max number right cameras supported (e.g. one or two, two would support a multi-baseline unit)
    const uint16_t MAX_NUM_INTRINSICS_THIRD = 3; // Max number native resolutions the third camera can have (e.g. 1920x1080 and 640x480)
    const uint16_t MAX_NUM_INTRINSICS_PLATFORM = 4; // Max number native resolutions the platform camera can have
    const uint16_t MAX_NUM_RECTIFIED_MODES_LR = 4; // Max number rectified LR resolution modes the structure supports (e.g. 640x480, 492x372 and 332x252)
    const uint16_t MAX_NUM_RECTIFIED_MODES_THIRD = 3; // Max number rectified Third resolution modes the structure supports (e.g. 1920x1080, 1280x720, etc)
    const uint16_t MAX_NUM_RECTIFIED_MODES_PLATFORM = 1; // Max number rectified Platform resolution modes the structure supports

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
    #pragma pack(pop)

    #define CAMERA_XU_UNIT_ID 2

    #define STATUS_BIT_Z_STREAMING (1 << 0)
    #define STATUS_BIT_LR_STREAMING (1 << 1)
    #define STATUS_BIT_WEB_STREAMING (1 << 2)
    #define STATUS_BIT_BOOT_DIAGNOSTIC_FAULT (1 << 3)
    #define STATUS_BIT_IFFLEY_CONSTANTS_VALID (1 << 4)
    #define STATUS_BIT_WATCHDOG_TIMER_RESET (1 << 5)
    #define STATUS_BIT_REC_BUFFER_OVERRUN (1 << 6)
    #define STATUS_BIT_CAM_DATA_FORMAT_ERROR (1 << 7)
    #define STATUS_BIT_CAM_FIFO_OVERFLOW (1 << 8)
    #define STATUS_BIT_REC_DIVIDED_BY_ZERO_ERROR (1 << 9)
    #define STATUS_BIT_UVC_HEADER_ERROR (1 << 10)
    #define STATUS_BIT_EMITTER_FAULT (1 << 11)
    #define STATUS_BIT_THERMAL_FAULT (1 << 12)
    #define STATUS_BIT_REC_RUN_ENABLED (1 << 13)
    #define STATUS_BIT_VDF_DEPTH_POINTER_STREAMING (1 << 14)
    #define STATUS_BIT_VDF_LR_POINTER_STREAMING (1 << 15)
    #define STATUS_BIT_VDF_WEBCAM_POINTER_STREAMING (1 << 16)
    #define STATUS_BIT_STREAMING_STATE (1 << 27) | (1 << 28) | (1 << 29) | (1 << 30)
    #define STATUS_BIT_BUSY (1 << 31)

    #define CONTROL_COMMAND_RESPONSE 1
    #define CONTROL_IFFLEY 2
    #define CONTROL_STREAM_INTENT 3
    #define CONTROL_DEPTH_UNITS 4
    #define CONTROL_MIN_MAX 5
    #define CONTROL_DISPARITY 6
    #define CONTROL_RECTIFICATION 7
    #define CONTROL_EMITTER 8
    #define CONTROL_TEMPERATURE 9
    #define CONTROL_DEPTH 10
    #define CONTROL_LAST_ERROR 12
    #define CONTROL_EMBEDDED_COUNT 13
    #define CONTROL_LR_EXPOSURE 14
    #define CONTROL_LR_AUTOEXPOSURE_PARAMETERS 15
    #define CONTROL_SW_RESET 16
    #define CONTROL_LR_GAIN 17
    #define CONTROL_LR_EXPOSURE_MODE 18
    #define CONTROL_DISPARITY_SHIFT 19
    #define CONTROL_STATUS 20
    #define CONTROL_LR_EXPOSURE_DISCOVERY 21
    #define CONTROL_LR_GAIN_DISCOVERY 22
    #define CONTROL_HW_TIMESTAMP 23

    #define COMMAND_DOWNLOAD_SPI_FLASH 0x1A
    #define COMMAND_PROTECT_FLASH 0x1C
    #define COMMAND_LED_ON 0x14
    #define COMMAND_LED_OFF 0x15
    #define COMMAND_GET_FWREVISION 0x21
    #define COMMAND_GET_SPI_PROTECT 0x23
    #define COMMAND_MODIFIER_DIRECT 0x00000010
    
} // end namespace r200
#endif

#endif
