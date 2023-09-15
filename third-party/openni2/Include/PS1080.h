/*****************************************************************************
*                                                                            *
*  OpenNI 2.x Alpha                                                          *
*  Copyright (C) 2012 PrimeSense Ltd.                                        *
*                                                                            *
*  This file is part of OpenNI.                                              *
*                                                                            *
*  Licensed under the Apache License, Version 2.0 (the "License");           *
*  you may not use this file except in compliance with the License.          *
*  You may obtain a copy of the License at                                   *
*                                                                            *
*      http://www.apache.org/licenses/LICENSE-2.0                            *
*                                                                            *
*  Unless required by applicable law or agreed to in writing, software       *
*  distributed under the License is distributed on an "AS IS" BASIS,         *
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  *
*  See the License for the specific language governing permissions and       *
*  limitations under the License.                                            *
*                                                                            *
*****************************************************************************/
#ifndef _PS1080_H_
#define _PS1080_H_

#include <OniCTypes.h>

/** The maximum permitted Xiron device name string length. */ 
#define XN_DEVICE_MAX_STRING_LENGTH 200

/* 
 * private properties of PS1080 devices.
 *
 * @remarks 
 * properties structure is 0x1080XXYY where XX is range and YY is code.
 * range values:
 * F0 - device properties
 * E0 - device commands
 * 00 - common stream properties
 * 10 - depth stream properties
 * 20 - color stream properties
 */
enum
{
	/*******************************************************************/
	/* Device properties                                               */
	/*******************************************************************/

	/** unsigned long long (XnSensorUsbInterface) */
	XN_MODULE_PROPERTY_USB_INTERFACE = 0x1080F001, // "UsbInterface"
	/** Boolean */ 
	XN_MODULE_PROPERTY_MIRROR = 0x1080F002, // "Mirror"
	/** unsigned long long, get only */
	XN_MODULE_PROPERTY_RESET_SENSOR_ON_STARTUP = 0x1080F004, // "ResetSensorOnStartup"
	/** unsigned long long, get only */
	XN_MODULE_PROPERTY_LEAN_INIT = 0x1080F005, // "LeanInit"
	/** char[XN_DEVICE_MAX_STRING_LENGTH], get only */
	XN_MODULE_PROPERTY_SERIAL_NUMBER = 0x1080F006, // "ID"
	/** XnVersions, get only */
	XN_MODULE_PROPERTY_VERSION = 0x1080F007, // "Version"
	/** Boolean */
	XN_MODULE_PROPERTY_FIRMWARE_FRAME_SYNC = 0x1080F008,
	/** Boolean */
	XN_MODULE_PROPERTY_HOST_TIMESTAMPS = 0x1080FF77, // "HostTimestamps"
	/** Boolean */
	XN_MODULE_PROPERTY_CLOSE_STREAMS_ON_SHUTDOWN = 0x1080FF78, // "CloseStreamsOnShutdown"
	/** Integer */
	XN_MODULE_PROPERTY_FIRMWARE_LOG_INTERVAL = 0x1080FF7F, // "FirmwareLogInterval"
	/** Boolean */
	XN_MODULE_PROPERTY_PRINT_FIRMWARE_LOG = 0x1080FF80, // "FirmwareLogPrint"
	/** Integer */
	XN_MODULE_PROPERTY_FIRMWARE_LOG_FILTER = 0x1080FF81, // "FirmwareLogFilter"
	/** String, get only */
	XN_MODULE_PROPERTY_FIRMWARE_LOG = 0x1080FF82, // "FirmwareLog"
	/** Integer */
	XN_MODULE_PROPERTY_FIRMWARE_CPU_INTERVAL = 0x1080FF83, // "FirmwareCPUInterval"
	/** String, get only */
	XN_MODULE_PROPERTY_PHYSICAL_DEVICE_NAME = 0x1080FF7A, // "PhysicalDeviceName"
	/** String, get only */
	XN_MODULE_PROPERTY_VENDOR_SPECIFIC_DATA = 0x1080FF7B, // "VendorSpecificData"
	/** String, get only */
	XN_MODULE_PROPERTY_SENSOR_PLATFORM_STRING = 0x1080FF7C, // "SensorPlatformString"

	/*******************************************************************/
	/* Device commands (activated via SetProperty/GetProperty)         */
	/*******************************************************************/

	/** XnInnerParam */
	XN_MODULE_PROPERTY_FIRMWARE_PARAM = 0x1080E001, // "FirmwareParam"
	/** unsigned long long, set only */
	XN_MODULE_PROPERTY_RESET = 0x1080E002, // "Reset"
	/** XnControlProcessingData */
	XN_MODULE_PROPERTY_IMAGE_CONTROL = 0x1080E003, // "ImageControl"
	/** XnControlProcessingData */
	XN_MODULE_PROPERTY_DEPTH_CONTROL = 0x1080E004, // "DepthControl"
	/** XnAHBData */
	XN_MODULE_PROPERTY_AHB = 0x1080E005, // "AHB"
	/** XnLedState */
	XN_MODULE_PROPERTY_LED_STATE = 0x1080E006, // "LedState"
	/** Boolean */
	XN_MODULE_PROPERTY_EMITTER_STATE = 0x1080E007, // "EmitterState"

	/** XnCmosBlankingUnits */
	XN_MODULE_PROPERTY_CMOS_BLANKING_UNITS = 0x1080FF74, // "CmosBlankingUnits"
	/** XnCmosBlankingTime */
	XN_MODULE_PROPERTY_CMOS_BLANKING_TIME = 0x1080FF75, // "CmosBlankingTime"
	/** XnFlashFileList, get only */
	XN_MODULE_PROPERTY_FILE_LIST = 0x1080FF84, // "FileList"
	/** XnParamFlashData, get only */
	XN_MODULE_PROPERTY_FLASH_CHUNK = 0x1080FF85, // "FlashChunk"
	XN_MODULE_PROPERTY_FILE = 0x1080FF86, // "FlashFile"
	/** Integer */
	XN_MODULE_PROPERTY_DELETE_FILE = 0x1080FF87, // "DeleteFile"
	XN_MODULE_PROPERTY_FILE_ATTRIBUTES = 0x1080FF88, // "FileAttributes"
	XN_MODULE_PROPERTY_TEC_SET_POINT = 0x1080FF89, // "TecSetPoint"
	/** get only */
	XN_MODULE_PROPERTY_TEC_STATUS = 0x1080FF8A, // "TecStatus"
	/** get only */
	XN_MODULE_PROPERTY_TEC_FAST_CONVERGENCE_STATUS = 0x1080FF8B, // "TecFastConvergenceStatus"
	XN_MODULE_PROPERTY_EMITTER_SET_POINT = 0x1080FF8C, // "EmitterSetPoint"
	/** get only */
	XN_MODULE_PROPERTY_EMITTER_STATUS = 0x1080FF8D, // "EmitterStatus"
	XN_MODULE_PROPERTY_I2C = 0x1080FF8E, // "I2C"
	/** Integer, set only */
	XN_MODULE_PROPERTY_BIST = 0x1080FF8F, // "BIST"
	/** XnProjectorFaultData, set only */
	XN_MODULE_PROPERTY_PROJECTOR_FAULT = 0x1080FF90, // "ProjectorFault"
	/** Boolean, set only */
	XN_MODULE_PROPERTY_APC_ENABLED = 0x1080FF91, // "APCEnabled"
	/** Boolean */
	XN_MODULE_PROPERTY_FIRMWARE_TEC_DEBUG_PRINT = 0x1080FF92, // "TecDebugPrint"

	/*******************************************************************/
	/* Common stream properties                                        */
	/*******************************************************************/

	/** unsigned long long */ 
	XN_STREAM_PROPERTY_INPUT_FORMAT = 0x10800001, // "InputFormat"
	/** unsigned long long (XnCroppingMode) */
	XN_STREAM_PROPERTY_CROPPING_MODE = 0x10800002, // "CroppingMode"

	/*******************************************************************/
	/* Depth stream properties                                         */
	/*******************************************************************/

	/** unsigned long long */
	XN_STREAM_PROPERTY_CLOSE_RANGE = 0x1080F003, // "CloseRange"
	/** XnPixelRegistration - get only */
	XN_STREAM_PROPERTY_PIXEL_REGISTRATION = 0x10801001, // "PixelRegistration"
	/** unsigned long long */
	XN_STREAM_PROPERTY_WHITE_BALANCE_ENABLED = 0x10801002, // "WhiteBalancedEnabled"
	/** unsigned long long */ 
	XN_STREAM_PROPERTY_GAIN = 0x10801003, // "Gain"
	/** unsigned long long */ 
	XN_STREAM_PROPERTY_HOLE_FILTER = 0x10801004, // "HoleFilter"
	/** unsigned long long (XnProcessingType) */ 
	XN_STREAM_PROPERTY_REGISTRATION_TYPE = 0x10801005, // "RegistrationType"
	/** XnDepthAGCBin* */
	XN_STREAM_PROPERTY_AGC_BIN = 0x10801006, // "AGCBin"
	/** unsigned long long, get only */ 
	XN_STREAM_PROPERTY_CONST_SHIFT = 0x10801007, // "ConstShift"
	/** unsigned long long, get only */ 
	XN_STREAM_PROPERTY_PIXEL_SIZE_FACTOR = 0x10801008, // "PixelSizeFactor"
	/** unsigned long long, get only */ 
	XN_STREAM_PROPERTY_MAX_SHIFT = 0x10801009, // "MaxShift"
	/** unsigned long long, get only */ 
	XN_STREAM_PROPERTY_PARAM_COEFF = 0x1080100A, // "ParamCoeff"
	/** unsigned long long, get only */ 
	XN_STREAM_PROPERTY_SHIFT_SCALE = 0x1080100B, // "ShiftScale"
	/** unsigned long long, get only */ 
	XN_STREAM_PROPERTY_ZERO_PLANE_DISTANCE = 0x1080100C, // "ZPD"
	/** double, get only */ 
	XN_STREAM_PROPERTY_ZERO_PLANE_PIXEL_SIZE = 0x1080100D, // "ZPPS"
	/** double, get only */ 
	XN_STREAM_PROPERTY_EMITTER_DCMOS_DISTANCE = 0x1080100E, // "LDDIS"
	/** double, get only */ 
	XN_STREAM_PROPERTY_DCMOS_RCMOS_DISTANCE = 0x1080100F, // "DCRCDIS"
	/** OniDepthPixel[], get only */ 
	XN_STREAM_PROPERTY_S2D_TABLE = 0x10801010, // "S2D"
	/** unsigned short[], get only */ 
	XN_STREAM_PROPERTY_D2S_TABLE = 0x10801011, // "D2S"
	/** get only */
	XN_STREAM_PROPERTY_DEPTH_SENSOR_CALIBRATION_INFO = 0x10801012,
	/** Boolean */
	XN_STREAM_PROPERTY_GMC_MODE	= 0x1080FF44, // "GmcMode"
	/** Boolean */
	XN_STREAM_PROPERTY_GMC_DEBUG = 0x1080FF45, // "GmcDebug"
	/** Boolean */
	XN_STREAM_PROPERTY_WAVELENGTH_CORRECTION = 0x1080FF46, // "WavelengthCorrection"
	/** Boolean */
	XN_STREAM_PROPERTY_WAVELENGTH_CORRECTION_DEBUG = 0x1080FF47, // "WavelengthCorrectionDebug"

	/*******************************************************************/
	/* Color stream properties                                         */
	/*******************************************************************/
	/** Integer */ 
	XN_STREAM_PROPERTY_FLICKER = 0x10802001, // "Flicker"
};

typedef enum 
{
	XN_SENSOR_FW_VER_UNKNOWN = 0,
	XN_SENSOR_FW_VER_0_17 = 1,
	XN_SENSOR_FW_VER_1_1 = 2,
	XN_SENSOR_FW_VER_1_2 = 3,
	XN_SENSOR_FW_VER_3_0 = 4,
	XN_SENSOR_FW_VER_4_0 = 5,
	XN_SENSOR_FW_VER_5_0 = 6,
	XN_SENSOR_FW_VER_5_1 = 7,
	XN_SENSOR_FW_VER_5_2 = 8,
	XN_SENSOR_FW_VER_5_3 = 9,
	XN_SENSOR_FW_VER_5_4 = 10,
	XN_SENSOR_FW_VER_5_5 = 11,
	XN_SENSOR_FW_VER_5_6 = 12,
	XN_SENSOR_FW_VER_5_7 = 13,
	XN_SENSOR_FW_VER_5_8 = 14,
} XnFWVer;

typedef enum {
	XN_SENSOR_VER_UNKNOWN = 0,
	XN_SENSOR_VER_2_0 = 1,
	XN_SENSOR_VER_3_0 = 2,
	XN_SENSOR_VER_4_0 = 3,
	XN_SENSOR_VER_5_0 = 4
} XnSensorVer;

typedef enum {
	XN_SENSOR_HW_VER_UNKNOWN = 0,
	XN_SENSOR_HW_VER_FPDB_10 = 1,
	XN_SENSOR_HW_VER_CDB_10  = 2,
	XN_SENSOR_HW_VER_RD_3  = 3,
	XN_SENSOR_HW_VER_RD_5  = 4,
	XN_SENSOR_HW_VER_RD1081  = 5,
	XN_SENSOR_HW_VER_RD1082  = 6,
	XN_SENSOR_HW_VER_RD109  = 7	
} XnHWVer;

typedef enum {
	XN_SENSOR_CHIP_VER_UNKNOWN = 0,
	XN_SENSOR_CHIP_VER_PS1000 = 1,
	XN_SENSOR_CHIP_VER_PS1080 = 2,
	XN_SENSOR_CHIP_VER_PS1080A6 = 3
} XnChipVer;

typedef enum
{
	XN_CMOS_TYPE_IMAGE = 0,
	XN_CMOS_TYPE_DEPTH = 1,

	XN_CMOS_COUNT
} XnCMOSType;

typedef enum
{
	XN_IO_IMAGE_FORMAT_BAYER = 0,
	XN_IO_IMAGE_FORMAT_YUV422 = 1,
	XN_IO_IMAGE_FORMAT_JPEG = 2,
	XN_IO_IMAGE_FORMAT_JPEG_420 = 3,
	XN_IO_IMAGE_FORMAT_JPEG_MONO = 4,
	XN_IO_IMAGE_FORMAT_UNCOMPRESSED_YUV422 = 5,
	XN_IO_IMAGE_FORMAT_UNCOMPRESSED_BAYER = 6,
	XN_IO_IMAGE_FORMAT_UNCOMPRESSED_YUYV = 7,
} XnIOImageFormats;

typedef enum
{
	XN_IO_DEPTH_FORMAT_UNCOMPRESSED_16_BIT = 0,
	XN_IO_DEPTH_FORMAT_COMPRESSED_PS = 1,
	XN_IO_DEPTH_FORMAT_UNCOMPRESSED_10_BIT = 2,
	XN_IO_DEPTH_FORMAT_UNCOMPRESSED_11_BIT = 3,
	XN_IO_DEPTH_FORMAT_UNCOMPRESSED_12_BIT = 4,
} XnIODepthFormats;

typedef enum
{
	XN_RESET_TYPE_POWER = 0,
	XN_RESET_TYPE_SOFT = 1,
	XN_RESET_TYPE_SOFT_FIRST = 2,
} XnParamResetType;

typedef enum XnSensorUsbInterface
{
	XN_SENSOR_USB_INTERFACE_DEFAULT = 0,
	XN_SENSOR_USB_INTERFACE_ISO_ENDPOINTS = 1,
	XN_SENSOR_USB_INTERFACE_BULK_ENDPOINTS = 2,
	XN_SENSOR_USB_INTERFACE_ISO_ENDPOINTS_LOW_DEPTH = 3,
} XnSensorUsbInterface;

typedef enum XnProcessingType
{
	XN_PROCESSING_DONT_CARE = 0,
	XN_PROCESSING_HARDWARE = 1,
	XN_PROCESSING_SOFTWARE = 2,
} XnProcessingType;

typedef enum XnCroppingMode
{
	XN_CROPPING_MODE_NORMAL = 1,
	XN_CROPPING_MODE_INCREASED_FPS = 2,
	XN_CROPPING_MODE_SOFTWARE_ONLY = 3,
} XnCroppingMode;

enum
{
	XN_ERROR_STATE_OK = 0,
	XN_ERROR_STATE_DEVICE_PROJECTOR_FAULT = 1,
	XN_ERROR_STATE_DEVICE_OVERHEAT = 2,
};

typedef enum XnFirmwareCroppingMode
{
	XN_FIRMWARE_CROPPING_MODE_DISABLED = 0,
	XN_FIRMWARE_CROPPING_MODE_NORMAL = 1,
	XN_FIRMWARE_CROPPING_MODE_INCREASED_FPS = 2,
} XnFirmwareCroppingMode;

typedef enum
{
	XnLogFilterDebug		= 0x0001,
	XnLogFilterInfo			= 0x0002,
	XnLogFilterError		= 0x0004,
	XnLogFilterProtocol		= 0x0008,
	XnLogFilterAssert		= 0x0010,
	XnLogFilterConfig		= 0x0020,
	XnLogFilterFrameSync	= 0x0040,
	XnLogFilterAGC			= 0x0080,
	XnLogFilterTelems		= 0x0100,

	XnLogFilterAll			= 0xFFFF
} XnLogFilter;

typedef enum
{
	XnFileAttributeReadOnly	= 0x8000
} XnFilePossibleAttributes;

typedef enum
{
	XnFlashFileTypeFileTable					= 0x00,
	XnFlashFileTypeScratchFile					= 0x01,
	XnFlashFileTypeBootSector					= 0x02,
	XnFlashFileTypeBootManager					= 0x03,
	XnFlashFileTypeCodeDownloader				= 0x04,
	XnFlashFileTypeMonitor						= 0x05,
	XnFlashFileTypeApplication					= 0x06,
	XnFlashFileTypeFixedParams					= 0x07,
	XnFlashFileTypeDescriptors					= 0x08,
	XnFlashFileTypeDefaultParams				= 0x09,
	XnFlashFileTypeImageCmos					= 0x0A,
	XnFlashFileTypeDepthCmos					= 0x0B,
	XnFlashFileTypeAlgorithmParams				= 0x0C,
	XnFlashFileTypeReferenceQVGA				= 0x0D,
	XnFlashFileTypeReferenceVGA					= 0x0E,
	XnFlashFileTypeMaintenance					= 0x0F,
	XnFlashFileTypeDebugParams					= 0x10,
	XnFlashFileTypePrimeProcessor				= 0x11,
	XnFlashFileTypeGainControl					= 0x12,
	XnFlashFileTypeRegistartionParams			= 0x13,
	XnFlashFileTypeIDParams						= 0x14,
	XnFlashFileTypeSensorTECParams				= 0x15,
	XnFlashFileTypeSensorAPCParams				= 0x16,
	XnFlashFileTypeSensorProjectorFaultParams	= 0x17,
	XnFlashFileTypeProductionFile				= 0x18,
	XnFlashFileTypeUpgradeInProgress			= 0x19,
	XnFlashFileTypeWavelengthCorrection			= 0x1A,
	XnFlashFileTypeGMCReferenceOffset			= 0x1B,
	XnFlashFileTypeSensorNESAParams				= 0x1C,
	XnFlashFileTypeSensorFault					= 0x1D,
	XnFlashFileTypeVendorData					= 0x1E,
} XnFlashFileType;

typedef enum XnBistType
{
	//Auto tests
	XN_BIST_IMAGE_CMOS = 1 << 0,
	XN_BIST_IR_CMOS = 1 << 1,
	XN_BIST_POTENTIOMETER = 1 << 2,
	XN_BIST_FLASH = 1 << 3,
	XN_BIST_FULL_FLASH = 1 << 4,
	XN_BIST_PROJECTOR_TEST_MASK = 1 << 5,
	XN_BIST_TEC_TEST_MASK = 1 << 6,

	// Manual tests
	XN_BIST_NESA_TEST_MASK = 1 << 7,
	XN_BIST_NESA_UNLIMITED_TEST_MASK = 1 << 8,

	// Mask of all the auto tests
	XN_BIST_ALL = (0xFFFFFFFF & ~XN_BIST_NESA_TEST_MASK & ~XN_BIST_NESA_UNLIMITED_TEST_MASK),

} XnBistType;

typedef enum XnBistError
{
	XN_BIST_RAM_TEST_FAILURE = 1 << 0,
	XN_BIST_IR_CMOS_CONTROL_BUS_FAILURE = 1 << 1,
	XN_BIST_IR_CMOS_DATA_BUS_FAILURE = 1 << 2,
	XN_BIST_IR_CMOS_BAD_VERSION = 1 << 3,
	XN_BIST_IR_CMOS_RESET_FAILUE = 1 << 4,
	XN_BIST_IR_CMOS_TRIGGER_FAILURE = 1 << 5,
	XN_BIST_IR_CMOS_STROBE_FAILURE = 1 << 6,
	XN_BIST_COLOR_CMOS_CONTROL_BUS_FAILURE = 1 << 7,
	XN_BIST_COLOR_CMOS_DATA_BUS_FAILURE = 1 << 8,
	XN_BIST_COLOR_CMOS_BAD_VERSION = 1 << 9,
	XN_BIST_COLOR_CMOS_RESET_FAILUE = 1 << 10,
	XN_BIST_FLASH_WRITE_LINE_FAILURE = 1 << 11,
	XN_BIST_FLASH_TEST_FAILURE = 1 << 12,
	XN_BIST_POTENTIOMETER_CONTROL_BUS_FAILURE = 1 << 13,
	XN_BIST_POTENTIOMETER_FAILURE = 1 << 14,
	XN_BIST_AUDIO_TEST_FAILURE = 1 << 15,
	XN_BIST_PROJECTOR_TEST_LD_FAIL = 1 << 16,
	XN_BIST_PROJECTOR_TEST_LD_FAILSAFE_TRIG_FAIL = 1 << 17,
	XN_BIST_PROJECTOR_TEST_FAILSAFE_HIGH_FAIL = 1 << 18,
	XN_BIST_PROJECTOR_TEST_FAILSAFE_LOW_FAIL = 1 << 19,
	XN_TEC_TEST_HEATER_CROSSED = 1 << 20,
	XN_TEC_TEST_HEATER_DISCONNETED = 1 << 21,
	XN_TEC_TEST_TEC_CROSSED = 1 << 22,
	XN_TEC_TEST_TEC_FAULT = 1 << 23,
} XnBistError;

typedef enum XnDepthCMOSType
{
	XN_DEPTH_CMOS_NONE = 0,
	XN_DEPTH_CMOS_MT9M001 = 1,
	XN_DEPTH_CMOS_AR130 = 2,
} XnDepthCMOSType;

typedef enum XnImageCMOSType
{
	XN_IMAGE_CMOS_NONE = 0,
	XN_IMAGE_CMOS_MT9M112 = 1,
	XN_IMAGE_CMOS_MT9D131 = 2,
	XN_IMAGE_CMOS_MT9M114 = 3,
} XnImageCMOSType;

#define XN_IO_MAX_I2C_BUFFER_SIZE 10
#define XN_MAX_LOG_SIZE	(6*1024)

#pragma pack (push, 1)

typedef struct XnSDKVersion
{
	unsigned char nMajor;
	unsigned char nMinor;
	unsigned char nMaintenance;
	unsigned short nBuild;
} XnSDKVersion;

typedef struct {
	unsigned char nMajor;
	unsigned char nMinor;
	unsigned short nBuild;
	unsigned int nChip;
	unsigned short nFPGA;
	unsigned short nSystemVersion;

	XnSDKVersion SDK;

	XnHWVer		HWVer;
	XnFWVer		FWVer;
	XnSensorVer SensorVer;
	XnChipVer	ChipVer;
} XnVersions;

typedef struct
{
	unsigned short nParam;
	unsigned short nValue;
} XnInnerParamData;

typedef struct XnDepthAGCBin 
{
	unsigned short nBin;
	unsigned short nMin;
	unsigned short nMax;
} XnDepthAGCBin;

typedef struct XnControlProcessingData
{
	unsigned short nRegister;
	unsigned short nValue;
} XnControlProcessingData;

typedef struct XnAHBData
{
	unsigned int nRegister;
	unsigned int nValue;
	unsigned int nMask;
} XnAHBData;

typedef struct XnPixelRegistration
{
	unsigned int nDepthX;
	unsigned int nDepthY;
	uint16_t nDepthValue;
	unsigned int nImageXRes;
	unsigned int nImageYRes;
	unsigned int nImageX; // out
	unsigned int nImageY; // out
} XnPixelRegistration;

typedef struct XnLedState
{
	uint16_t nLedID;
	uint16_t nState;
} XnLedState;

typedef struct XnCmosBlankingTime
{
	XnCMOSType nCmosID;
	float nTimeInMilliseconds;
	uint16_t nNumberOfFrames;
} XnCmosBlankingTime;

typedef struct XnCmosBlankingUnits
{
	XnCMOSType nCmosID;
	uint16_t nUnits;
	uint16_t nNumberOfFrames;
} XnCmosBlankingUnits;

typedef struct XnI2CWriteData
{
	uint16_t nBus;
	uint16_t nSlaveAddress;
	uint16_t cpWriteBuffer[XN_IO_MAX_I2C_BUFFER_SIZE];
	uint16_t nWriteSize;
} XnI2CWriteData;

typedef struct XnI2CReadData
{
	uint16_t nBus;
	uint16_t nSlaveAddress;
	uint16_t cpReadBuffer[XN_IO_MAX_I2C_BUFFER_SIZE];
	uint16_t cpWriteBuffer[XN_IO_MAX_I2C_BUFFER_SIZE];
	uint16_t nReadSize;
	uint16_t nWriteSize;
} XnI2CReadData;

typedef struct XnTecData
{
	uint16_t m_SetPointVoltage;
	uint16_t m_CompensationVoltage;
	uint16_t m_TecDutyCycle; //duty cycle on heater/cooler
	uint16_t m_HeatMode; //TRUE - heat, FALSE - cool
	int32_t m_ProportionalError;
	int32_t m_IntegralError;
	int32_t m_DerivativeError;
	uint16_t m_ScanMode; //0 - crude, 1 - precise
} XnTecData;

typedef struct XnTecFastConvergenceData
{
	int16_t     m_SetPointTemperature;  // set point temperature in celsius,
	// scaled by factor of 100 (extra precision)
	int16_t     m_MeasuredTemperature;  // measured temperature in celsius,
	// scaled by factor of 100 (extra precision)
	int32_t 	m_ProportionalError;    // proportional error in system clocks
	int32_t 	m_IntegralError;        // integral error in system clocks
	int32_t 	m_DerivativeError;      // derivative error in system clocks
	uint16_t 	m_ScanMode; // 0 - initial, 1 - crude, 2 - precise
	uint16_t    m_HeatMode; // 0 - idle, 1 - heat, 2 - cool
	uint16_t    m_TecDutyCycle; // duty cycle on heater/cooler in percents
	uint16_t	m_TemperatureRange;	// 0 - cool, 1 - room, 2 - warm
} XnTecFastConvergenceData;

typedef struct XnEmitterData
{
	uint16_t m_State; //idle, calibrating
	uint16_t m_SetPointVoltage; //this is what should be written to the XML
	uint16_t m_SetPointClocks; //target cross duty cycle
	uint16_t m_PD_Reading; //current cross duty cycle in system clocks(high time)
	uint16_t m_EmitterSet; //duty cycle on emitter set in system clocks (high time).
	uint16_t m_EmitterSettingLogic; //TRUE = positive logic, FALSE = negative logic
	uint16_t m_LightMeasureLogic; //TRUE - positive logic, FALSE - negative logic
	uint16_t m_IsAPCEnabled;
	uint16_t m_EmitterSetStepSize; // in MilliVolts
	uint16_t m_ApcTolerance; // in system clocks (only valid up till v5.2)
	uint16_t m_SubClocking; //in system clocks (only valid from v5.3)
	uint16_t m_Precision; // (only valid from v5.3)
} XnEmitterData;

typedef struct
{
	uint16_t nId;
	uint16_t nAttribs;
} XnFileAttributes;

typedef struct
{
	uint32_t nOffset;
	const char* strFileName;
	uint16_t nAttributes;
} XnParamFileData;

typedef struct
{
	uint32_t nOffset;
	uint32_t nSize;
	unsigned char* pData;
} XnParamFlashData;

typedef struct  {
	uint16_t nId;
	uint16_t nType;
	uint32_t nVersion;
	uint32_t nOffset;
	uint32_t nSize;
	uint16_t nCrc;
	uint16_t nAttributes;
	uint16_t nReserve;
} XnFlashFile;

typedef struct  
{
	XnFlashFile* pFiles;
	uint16_t nFiles;
} XnFlashFileList;

typedef struct XnProjectorFaultData
{
	uint16_t nMinThreshold;
	uint16_t nMaxThreshold;
	int32_t bProjectorFaultEvent;
} XnProjectorFaultData;

typedef struct XnBist
{
	uint32_t nTestsMask;
	uint32_t nFailures;
} XnBist;

#pragma pack (pop)

#endif //_PS1080_H_