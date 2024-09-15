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
#ifndef _PRIME_SENSE_H_
#define _PRIME_SENSE_H_

#include <OniCTypes.h>

/**
* Additional properties for PrimeSense devices
*
* @remarks 
* properties structure is 0x1D27XXYY where XX is range and YY is code.
* range values:
* 00 - common stream properties
* 10 - depth stream properties
* E0 - device commands
* F0 - device properties
*/
enum
{
	// Stream Properties
	PS_PROPERTY_DUMP_DATA = 0x1d270001, // boolean

	// Device Properties
	PS_PROPERTY_USB_INTERFACE = 0x1d27F001, // values from XnUsbInterfaceType
};

/**
* Additional commands for PrimeSense devices
*
* @remarks 
* Commands structure is 0x1D27XXYY where XX is range and YY is code.
* range values:
* E0 - device commands
*/
enum
{
	// Device Commands - use via invoke()
	PS_COMMAND_AHB_READ = 0x1d27E001, // XnCommandAHB
	PS_COMMAND_AHB_WRITE = 0x1d27E002, // XnCommandAHB
	PS_COMMAND_I2C_READ = 0x1d27E003, // XnCommandI2C
	PS_COMMAND_I2C_WRITE = 0x1d27E004, // XnCommandI2C
	PS_COMMAND_SOFT_RESET = 0x1d27E005, // no arguments
	PS_COMMAND_POWER_RESET = 0x1d27E006, // no arguments
	PS_COMMAND_BEGIN_FIRMWARE_UPDATE = 0x1d27E007, // no arguments
	PS_COMMAND_END_FIRMWARE_UPDATE = 0x1d27E008, // no arguments
	PS_COMMAND_UPLOAD_FILE = 0x1d27E009, // XnCommandUploadFile
	PS_COMMAND_DOWNLOAD_FILE = 0x1d27E00A, // XnCommandDownloadFile
	PS_COMMAND_GET_FILE_LIST = 0x1d27E00B, // an array of XnFileEntry
	PS_COMMAND_FORMAT_ZONE = 0x1d27E00C, // XnCommandFormatZone
	PS_COMMAND_DUMP_ENDPOINT = 0x1d27E00D, // XnCommandDumpEndpoint
	PS_COMMAND_GET_I2C_DEVICE_LIST = 0x1d27E00E, // XnCommandGetI2CDevices
	PS_COMMAND_GET_BIST_LIST = 0x1d27E00F, // XnCommandGetBistList
	PS_COMMAND_EXECUTE_BIST = 0x1d27E010, // XnCommandExecuteBist
	PS_COMMAND_USB_TEST = 0x1d27E011, // XnCommandUsbTest
	PS_COMMAND_GET_LOG_MASK_LIST = 0x1d27E012, // XnCommandGetLogMaskList
	PS_COMMAND_SET_LOG_MASK_STATE = 0x1d27E013, // XnCommandSetLogMaskState
	PS_COMMAND_START_LOG = 0x1d27E014, // no arguments
	PS_COMMAND_STOP_LOG = 0x1d27E015, // no arguments
};

typedef enum XnUsbInterfaceType
{
	PS_USB_INTERFACE_DONT_CARE = 0,
	PS_USB_INTERFACE_ISO_ENDPOINTS = 1,
	PS_USB_INTERFACE_BULK_ENDPOINTS = 2,
} XnUsbInterfaceType;

#pragma pack (push, 1)

// Data Types
typedef struct XnFwFileVersion
{
	uint8_t major;
	uint8_t minor;
	uint8_t maintenance;
	uint8_t build;
} XnFwFileVersion;

typedef enum XnFwFileFlags
{
	XN_FILE_FLAG_BAD_CRC					= 0x0001,
} XnFwFileFlags;

typedef struct XnFwFileEntry
{
	char name[32];
	XnFwFileVersion version;
	uint32_t address;
	uint32_t size;
	uint16_t crc;
	uint16_t zone;
	XnFwFileFlags flags; // bitmap 
} XnFwFileEntry;

typedef struct XnI2CDeviceInfo
{
	uint32_t id;
	char name[32];
} XnI2CDeviceInfo;

typedef struct XnBistInfo
{
	uint32_t id;
	char name[32];
} XnBistInfo;

typedef struct XnFwLogMask
{
	uint32_t id;
	char name[32];
} XnFwLogMask;

typedef struct XnUsbTestEndpointResult
{
	double averageBytesPerSecond;
	uint32_t lostPackets;
} XnUsbTestEndpointResult;

// Commands

typedef struct XnCommandAHB
{
	uint32_t address;		// Address of this register
	uint32_t offsetInBits;	// Offset of the field in bits within address
	uint32_t widthInBits;	// Width of the field in bits
	uint32_t value;			// For read requests, this is where the actual value will be filled. For write requests, the value to write.
} XnCommandAHB;

typedef struct XnCommandI2C
{
	uint32_t deviceID;		// Device to communicate with
	uint32_t addressSize;	// Size of the address, in bytes (1-4)
	uint32_t address;		// Address
	uint32_t valueSize;		// Size of the value, in bytes (1-4)
	uint32_t mask;			// For write request - a mask to be applied to the value. For read requests - ignored.
	uint32_t value;			// For write request - the value to be written. For read requests - the place where the actual value is written to
} XnCommandI2C;

typedef struct XnCommandUploadFile  
{
	const char* filePath;
	uint32_t uploadToFactory;
} XnCommandUploadFile;  

typedef struct XnCommandDownloadFile  
{
	uint16_t zone;
	const char*  firmwareFileName;
	const char*  targetPath;
} XnCommandDownloadFile;  

typedef struct XnCommandGetFileList
{
	uint32_t count;		// in: number of allocated elements in files array. out: number of written elements in the array
	XnFwFileEntry* files;
} XnCommandGetFileList;

typedef struct XnCommandFormatZone 
{
	uint8_t zone;
} XnCommandFormatZone;

typedef struct XnCommandDumpEndpoint
{
	uint8_t endpoint;
	bool enabled;
} XnCommandDumpEndpoint;

typedef struct XnCommandGetI2CDeviceList
{
	uint32_t count;	// in: number of allocated elements in devices array. out: number of written elements in the array
	XnI2CDeviceInfo* devices;
} XnCommandGetI2CDeviceList;

typedef struct XnCommandGetBistList
{
	uint32_t count;	// in: number of allocated elements in tests array. out: number of written elements in the array
	XnBistInfo* tests;
} XnCommandGetBistList;

typedef struct XnCommandExecuteBist
{
	uint32_t id;
	uint32_t errorCode;
	uint32_t extraDataSize;	// in: number of allocated bytes in extraData. out: number of written bytes in extraData
	uint8_t* extraData;
} XnCommandExecuteBist;

typedef struct XnCommandUsbTest
{
	uint32_t seconds;
	uint32_t endpointCount;	// in: number of allocated bytes in endpoints array. out: number of written bytes in array
	XnUsbTestEndpointResult* endpoints;
} XnCommandUsbTest;

typedef struct XnCommandGetLogMaskList
{
	uint32_t count;	// in: number of allocated elements in masks array. out: number of written elements in the array
	XnFwLogMask* masks;
} XnCommandGetLogMaskList;

typedef struct XnCommandSetLogMaskState
{
	uint32_t mask;
	bool enabled;
} XnCommandSetLogMaskState;

#pragma pack (pop)

#endif //_PRIME_SENSE_H_