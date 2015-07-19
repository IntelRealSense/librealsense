#ifndef R200_CAMERA_HEADER_H
#define R200_CAMERA_HEADER_H
#include <cstdint>

namespace R200
{
    
#pragma pack(1)
#define CURRENT_CAMERA_CONTENTS_VERSION_NUMBER 10
    
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
    uint8_t reserved2[4];
    uint32_t emitterType;
    uint8_t reserved3[4];
    uint32_t cameraFPGAVersion;
    uint8_t reserved4[4];
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
    uint32_t theLastWord;
    uint8_t reserved5[57];
} __attribute__((aligned(1)));
    
#pragma pack()
    
} // R200

#endif
