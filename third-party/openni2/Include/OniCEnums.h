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
#ifndef _ONI_C_ENUMS_H_
#define _ONI_C_ENUMS_H_

/** Possible failure values */
typedef enum
{
	ONI_STATUS_OK = 0,
	ONI_STATUS_ERROR = 1,
	ONI_STATUS_NOT_IMPLEMENTED = 2,
	ONI_STATUS_NOT_SUPPORTED = 3,
	ONI_STATUS_BAD_PARAMETER = 4,
	ONI_STATUS_OUT_OF_FLOW = 5,
	ONI_STATUS_NO_DEVICE = 6,
	ONI_STATUS_TIME_OUT = 102,
} OniStatus;

/** The source of the stream */
typedef enum
{
	ONI_SENSOR_IR = 1,
	ONI_SENSOR_COLOR = 2,
	ONI_SENSOR_DEPTH = 3,

} OniSensorType;

/** All available formats of the output of a stream */
typedef enum
{
	// Depth
	ONI_PIXEL_FORMAT_DEPTH_1_MM = 100,
	ONI_PIXEL_FORMAT_DEPTH_100_UM = 101,
	ONI_PIXEL_FORMAT_SHIFT_9_2 = 102,
	ONI_PIXEL_FORMAT_SHIFT_9_3 = 103,

	// Color
	ONI_PIXEL_FORMAT_RGB888 = 200,
	ONI_PIXEL_FORMAT_YUV422 = 201,
	ONI_PIXEL_FORMAT_GRAY8 = 202,
	ONI_PIXEL_FORMAT_GRAY16 = 203,
	ONI_PIXEL_FORMAT_JPEG = 204,
	ONI_PIXEL_FORMAT_YUYV = 205,
} OniPixelFormat;

typedef enum
{
	ONI_DEVICE_STATE_OK 		= 0,
	ONI_DEVICE_STATE_ERROR		= 1,
	ONI_DEVICE_STATE_NOT_READY 	= 2,
	ONI_DEVICE_STATE_EOF 		= 3
} OniDeviceState;

typedef enum
{
	ONI_IMAGE_REGISTRATION_OFF				= 0,
	ONI_IMAGE_REGISTRATION_DEPTH_TO_COLOR	= 1,
} OniImageRegistrationMode;

enum
{
	ONI_TIMEOUT_NONE = 0,
	ONI_TIMEOUT_FOREVER = -1,
};

#endif // _ONI_C_ENUMS_H_
