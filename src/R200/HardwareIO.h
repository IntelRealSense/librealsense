#pragma once

#ifndef LIBREALSENSE_R200_HARDWARE_IO_H
#define LIBREALSENSE_R200_HARDWARE_IO_H

#include <libuvc/libuvc.h>
#include <memory>

#include "XU.h"

namespace r200
{
    struct CameraCalibrationParameters;
    struct CameraHeaderInfo;
    class DS4HardwareIOInternal;

    class DS4HardwareIO
    {
        std::unique_ptr<DS4HardwareIOInternal> internal;
    public:

        DS4HardwareIO(uvc_device_handle_t * deviceHandle);
        ~DS4HardwareIO();

        CameraCalibrationParameters GetCalibration();
        CameraHeaderInfo GetCameraHeader();
    };

    #pragma pack(push, 1)
    const uint16_t MAX_NUM_INTRINSICS_RIGHT = 2; // Max number right cameras supported (e.g. one or two, two would support a multi-baseline unit)
    const uint16_t MAX_NUM_INTRINSICS_THIRD = 3; // Max number native resolutions the third camera can have (e.g. 1920x1080 and 640x480)
    const uint16_t MAX_NUM_INTRINSICS_PLATFORM = 4; // Max number native resolutions the platform camera can have
    const uint16_t MAX_NUM_RECTIFIED_MODES_LR = 4; // Max number rectified LR resolution modes the structure supports (e.g. 640x480, 492x372 and 332x252)
    const uint16_t MAX_NUM_RECTIFIED_MODES_THIRD = 3; // Max number rectified Third resolution modes the structure supports (e.g. 1920x1080, 1280x720, etc)
    const uint16_t MAX_NUM_RECTIFIED_MODES_PLATFORM = 1; // Max number rectified Platform resolution modes the structure supports

    struct CalibrationMetadata
    {
        uint32_t versionNumber;
        uint16_t numIntrinsicsRight;
        uint16_t numIntrinsicsThird;
        uint16_t numIntrinsicsPlatform;
        uint16_t numRectifiedModesLR;
        uint16_t numRectifiedModesThird;
        uint16_t numRectifiedModesPlatform;
    };

    struct UnrectifiedIntrinsics
    {
        float fx;
        float fy;
        float px;
        float py;
        float k[5];
        uint32_t w;
        uint32_t h;
    };

    struct RectifiedIntrinsics
    {
        float rfx;
        float rfy;
        float rpx;
        float rpy;
        uint32_t rw;
        uint32_t rh;
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

        float Rleft[MAX_NUM_INTRINSICS_RIGHT][9];
        float Rright[MAX_NUM_INTRINSICS_RIGHT][9];
        float Rthird[MAX_NUM_INTRINSICS_RIGHT][9];
        float Rplatform[MAX_NUM_INTRINSICS_RIGHT][9];

        float B[MAX_NUM_INTRINSICS_RIGHT];
        float T[MAX_NUM_INTRINSICS_RIGHT][3];
        float Tplatform[MAX_NUM_INTRINSICS_RIGHT][3];

        float Rworld[9];
        float Tworld[3];
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
} // end namespace r200

#endif


