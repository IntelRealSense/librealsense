#ifndef __XN_PRIME_CLIENT_PROPS_H__
#define __XN_PRIME_CLIENT_PROPS_H__

#include <PrimeSense.h>

enum
{
	/**** Device properties ****/

	/* XnDetailedVersion, get only */
	LINK_PROP_FW_VERSION = 0x12000001, // "FWVersion"
	/* Int, get only */
	LINK_PROP_VERSIONS_INFO_COUNT = 0x12000002, // "VersionsInfoCount"
	/* General - array - XnComponentVersion * count elements, get only */
	LINK_PROP_VERSIONS_INFO = 0x12000003, // "VersionsInfo"
	/* Int - 0 means off, 1 means on. */
	LINK_PROP_EMITTER_ACTIVE = 0x12000008, // "EmitterActive"
	/* String. Set only */
	LINK_PROP_PRESET_FILE = 0x1200000a, // "PresetFile"
	/* Get only */
	LINK_PROP_BOOT_STATUS = 0x1200000b,

	/**** Device commands ****/
	/* XnCommandGetFwStreams */
	LINK_COMMAND_GET_FW_STREAM_LIST = 0x1200F001,
	/* XnCommandCreateStream */
	LINK_COMMAND_CREATE_FW_STREAM = 0x1200F002,
	/* XnCommandDestroyStream */
	LINK_COMMAND_DESTROY_FW_STREAM = 0x1200F003,
	/* XnCommandStartStream */
	LINK_COMMAND_START_FW_STREAM = 0x1200F004,
	/* XnCommandStopStream */
	LINK_COMMAND_STOP_FW_STREAM = 0x1200F005,
	/* XnCommandGetFwStreamVideoModeList */
	LINK_COMMAND_GET_FW_STREAM_VIDEO_MODE_LIST = 0x1200F006,
	/* XnCommandSetFwStreamVideoMode */
	LINK_COMMAND_SET_FW_STREAM_VIDEO_MODE = 0x1200F007,
	/* XnCommandGetFwStreamVideoMode */
	LINK_COMMAND_GET_FW_STREAM_VIDEO_MODE = 0x1200F008,

	/**** Stream properties ****/
	/* Int. 1 - Shifts 9.3, 2 - Grayscale16, 3 - YUV422, 4 - Bayer8 */
	LINK_PROP_PIXEL_FORMAT = 0x12001001, // "PixelFormat"
	/* Int. 0 - None, 1 - 8z, 2 - 16z, 3 - 24z, 4 - 6-bit, 5 - 10-bit, 6 - 11-bit, 7 - 12-bit */
	LINK_PROP_COMPRESSION = 0x12001002, // "Compression"

	/**** Depth Stream properties ****/
	/* Real, get only */
	LINK_PROP_DEPTH_SCALE = 0x1200000b, // "DepthScale"
	/* Int, get only */
	LINK_PROP_MAX_SHIFT = 0x12002001, // "MaxShift"
	/* Int, get only */
	LINK_PROP_ZERO_PLANE_DISTANCE = 0x12002002, // "ZPD"
	/* Int, get only */
	LINK_PROP_CONST_SHIFT = 0x12002003, // "ConstShift"
	/* Int, get only */
	LINK_PROP_PARAM_COEFF = 0x12002004, // "ParamCoeff"
	/* Int, get only */
	LINK_PROP_SHIFT_SCALE = 0x12002005, // "ShiftScale"
	/* Real, get only */
	LINK_PROP_ZERO_PLANE_PIXEL_SIZE = 0x12002006, // "ZPPS"
	/* Real, get only */
	LINK_PROP_ZERO_PLANE_OUTPUT_PIXEL_SIZE = 0x12002007, // "ZPOPS"
	/* Real, get only */
	LINK_PROP_EMITTER_DEPTH_CMOS_DISTANCE = 0x12002008, // "LDDIS"
	/*  General - array - MaxShift * XnDepthPixel elements, get only */
	LINK_PROP_SHIFT_TO_DEPTH_TABLE = 0x12002009, // "S2D"
	/* General - array - MaxDepth * uint16_t elements, get only */
	LINK_PROP_DEPTH_TO_SHIFT_TABLE = 0x1200200a, // "D2S"
};

typedef enum XnFileZone
{
	XN_ZONE_FACTORY                 = 0x0000,
	XN_ZONE_UPDATE                  = 0x0001,
} XnFileZone;

typedef enum XnBootErrorCode
{
	XN_BOOT_OK                      = 0x0000,
	XN_BOOT_BAD_CRC                 = 0x0001,
	XN_BOOT_UPLOAD_IN_PROGRESS      = 0x0002,
	XN_BOOT_FW_LOAD_FAILED          = 0x0003,
} XnBootErrorCode;

typedef enum XnFwStreamType
{
	XN_FW_STREAM_TYPE_COLOR			= 0x0001,
	XN_FW_STREAM_TYPE_IR			= 0x0002,
	XN_FW_STREAM_TYPE_SHIFTS		= 0x0003,
	XN_FW_STREAM_TYPE_AUDIO			= 0x0004,
	XN_FW_STREAM_TYPE_DY			= 0x0005,
	XN_FW_STREAM_TYPE_LOG			= 0x0008,
} XnFwStreamType;

typedef enum XnFwPixelFormat
{
	XN_FW_PIXEL_FORMAT_NONE			= 0x0000,
	XN_FW_PIXEL_FORMAT_SHIFTS_9_3	= 0x0001,
	XN_FW_PIXEL_FORMAT_GRAYSCALE16	= 0x0002,
	XN_FW_PIXEL_FORMAT_YUV422		= 0x0003,
	XN_FW_PIXEL_FORMAT_BAYER8		= 0x0004,
} XnFwPixelFormat;

typedef enum XnFwCompressionType
{
	XN_FW_COMPRESSION_NONE			= 0x0000,
	XN_FW_COMPRESSION_8Z			= 0x0001,
	XN_FW_COMPRESSION_16Z			= 0x0002,
	XN_FW_COMPRESSION_24Z			= 0x0003,
	XN_FW_COMPRESSION_6_BIT_PACKED	= 0x0004,
	XN_FW_COMPRESSION_10_BIT_PACKED	= 0x0005,
	XN_FW_COMPRESSION_11_BIT_PACKED = 0x0006,
	XN_FW_COMPRESSION_12_BIT_PACKED	= 0x0007,
} XnFwCompressionType;

#pragma pack (push, 1)

#define XN_MAX_VERSION_MODIFIER_LENGTH 16
typedef struct XnDetailedVersion
{
	uint8_t m_nMajor;
	uint8_t m_nMinor;
	uint16_t m_nMaintenance;
	uint32_t m_nBuild;
	char m_strModifier[XN_MAX_VERSION_MODIFIER_LENGTH];
} XnDetailedVersion;

typedef struct XnBootStatus
{
	XnFileZone zone;
	XnBootErrorCode errorCode;
} XnBootStatus;

typedef struct XnFwStreamInfo
{
	XnFwStreamType type;
	char creationInfo[80];
} XnFwStreamInfo;

typedef struct XnFwStreamVideoMode
{
	uint32_t m_nXRes;
	uint32_t m_nYRes;
	uint32_t m_nFPS;
	XnFwPixelFormat m_nPixelFormat;
	XnFwCompressionType m_nCompression;
} XnFwStreamVideoMode;

typedef struct XnCommandGetFwStreamList
{
	uint32_t count;	// in: number of allocated elements in streams array. out: number of written elements in the array
	XnFwStreamInfo* streams;
} XnCommandGetFwStreamList;

typedef struct XnCommandCreateStream
{
	XnFwStreamType type;
	const char* creationInfo;
	uint32_t id; // out
} XnCommandCreateStream;

typedef struct XnCommandDestroyStream
{
	uint32_t id;
} XnCommandDestroyStream;

typedef struct XnCommandStartStream
{
	uint32_t id;
} XnCommandStartStream;

typedef struct XnCommandStopStream
{
	uint32_t id;
} XnCommandStopStream;

typedef struct XnCommandGetFwStreamVideoModeList
{
	int streamId;
	uint32_t count;	// in: number of allocated elements in videoModes array. out: number of written elements in the array
	XnFwStreamVideoMode* videoModes;
} XnCommandGetFwStreamVideoModeList;

typedef struct XnCommandSetFwStreamVideoMode
{
	int streamId;
	XnFwStreamVideoMode videoMode;
} XnCommandSetFwStreamVideoMode;

typedef struct XnCommandGetFwStreamVideoMode
{
	int streamId;
	XnFwStreamVideoMode videoMode; // out
} XnCommandGetFwStreamVideoMode;

#pragma pack (pop)

#endif //__XN_PRIME_CLIENT_PROPS_H__
