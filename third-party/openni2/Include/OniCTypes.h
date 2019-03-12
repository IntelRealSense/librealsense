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
#ifndef _ONI_TYPES_H_
#define _ONI_TYPES_H_

#include "OniPlatform.h"
#include "OniCEnums.h"

/** Basic types **/
typedef int OniBool;

#ifndef TRUE
#define TRUE 1
#endif //TRUE
#ifndef FALSE
#define FALSE 0
#endif //FALSE

#define ONI_MAX_STR 256
#define ONI_MAX_SENSORS 10

struct OniCallbackHandleImpl;
typedef struct OniCallbackHandleImpl* OniCallbackHandle;

/** Holds an OpenNI version number, which consists of four separate numbers in the format: @c major.minor.maintenance.build. For example: 2.0.0.20. */
typedef struct
{
	/** Major version number, incremented for major API restructuring. */
	int major;
	/** Minor version number, incremented when significant new features added. */
	int minor;
	/** Maintenance build number, incremented for new releases that primarily provide minor bug fixes. */
	int maintenance;
	/** Build number. Incremented for each new API build. Generally not shown on the installer and download site. */
	int build;
} OniVersion;

typedef int OniHardwareVersion;

/** Description of the output: format and resolution */
typedef struct
{
	OniPixelFormat pixelFormat;
	int resolutionX;
	int resolutionY;
	int fps;
} OniVideoMode;

/** List of supported video modes by a specific source */
typedef struct
{
	OniSensorType sensorType;
	int numSupportedVideoModes;
	OniVideoMode *pSupportedVideoModes;
} OniSensorInfo;

/** Basic description of a device */
typedef struct
{
	char uri[ONI_MAX_STR];
	char vendor[ONI_MAX_STR];
	char name[ONI_MAX_STR];
	uint16_t usbVendorId;
	uint16_t usbProductId;
} OniDeviceInfo;

struct _OniDevice;
typedef _OniDevice* OniDeviceHandle;

struct _OniStream;
typedef _OniStream* OniStreamHandle;

struct _OniRecorder;
typedef _OniRecorder* OniRecorderHandle;

/** All information of the current frame */
typedef struct
{
	int dataSize;
	void* data;

	OniSensorType sensorType;
	uint64_t timestamp;
	int frameIndex;

	int width;
	int height;

	OniVideoMode videoMode;
	OniBool croppingEnabled;
	int cropOriginX;
	int cropOriginY;

	int stride;
} OniFrame;

typedef void (ONI_CALLBACK_TYPE* OniNewFrameCallback)(OniStreamHandle stream, void* pCookie);
typedef void (ONI_CALLBACK_TYPE* OniGeneralCallback)(void* pCookie);
typedef void (ONI_CALLBACK_TYPE* OniDeviceInfoCallback)(const OniDeviceInfo* pInfo, void* pCookie);
typedef void (ONI_CALLBACK_TYPE* OniDeviceStateCallback)(const OniDeviceInfo* pInfo, OniDeviceState deviceState, void* pCookie);

typedef void* (ONI_CALLBACK_TYPE* OniFrameAllocBufferCallback)(int size, void* pCookie);
typedef void (ONI_CALLBACK_TYPE* OniFrameFreeBufferCallback)(void* data, void* pCookie);

typedef struct
{
	OniDeviceInfoCallback		deviceConnected;
	OniDeviceInfoCallback		deviceDisconnected;
	OniDeviceStateCallback		deviceStateChanged;
} OniDeviceCallbacks;

typedef struct
{
	int enabled;
	int originX;
	int originY;
	int width;
	int height;
} OniCropping;

// Pixel types
/**
Pixel type used to store depth images.
*/
typedef uint16_t OniDepthPixel;

/**
Pixel type used to store 16-bit grayscale images
*/
typedef uint16_t OniGrayscale16Pixel;

/**
Pixel type used to store 8-bit grayscale/bayer images
*/
typedef uint8_t OniGrayscale8Pixel;

#pragma pack (push, 1)

/** Holds the value of a single color image pixel in 24-bit RGB format. */
typedef struct
{
	/* Red value of this pixel. */
	uint8_t r;
	/* Green value of this pixel. */
	uint8_t g;
	/* Blue value of this pixel. */
	uint8_t b;
} OniRGB888Pixel;

/**
 Holds the value of two pixels in YUV422 format (Luminance/Chrominance,16-bits/pixel).
 The first pixel has the values y1, u, v.
 The second pixel has the values y2, u, v.
*/
typedef struct
{
	/** First chrominance value for two pixels, stored as blue luminance difference signal. */
	uint8_t u;
	/** Overall luminance value of first pixel. */
	uint8_t y1;
	/** Second chrominance value for two pixels, stored as red luminance difference signal. */
	uint8_t v;
	/** Overall luminance value of second pixel. */
	uint8_t y2;
} OniYUV422DoublePixel;

#pragma pack (pop)

typedef struct
{
	int frameIndex;
	OniStreamHandle stream;
} OniSeek;

#endif // _ONI_TYPES_H_
