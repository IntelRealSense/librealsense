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
#ifndef _ONI_DRIVER_TYPES_H_
#define _ONI_DRIVER_TYPES_H_

#include <OniCTypes.h>
#include <stdarg.h>

#define ONI_STREAM_PROPERTY_PRIVATE_BASE XN_MAX_UINT16

typedef struct
{
	int dataSize;
	void* data;
} OniGeneralBuffer;

/////// DriverServices
struct OniDriverServices
{
	void* driverServices;
	void (ONI_CALLBACK_TYPE* errorLoggerAppend)(void* driverServices, const char* format, va_list args);
	void (ONI_CALLBACK_TYPE* errorLoggerClear)(void* driverServices);
	void (ONI_CALLBACK_TYPE* log)(void* driverServices, int severity, const char* file, int line, const char* mask, const char* message);
};

struct OniStreamServices
{
	void* streamServices;
	int (ONI_CALLBACK_TYPE* getDefaultRequiredFrameSize)(void* streamServices);
	OniFrame* (ONI_CALLBACK_TYPE* acquireFrame)(void* streamServices); // returns a frame with size corresponding to getRequiredFrameSize()
	void (ONI_CALLBACK_TYPE* addFrameRef)(void* streamServices, OniFrame* pframe);
	void (ONI_CALLBACK_TYPE* releaseFrame)(void* streamServices, OniFrame* pframe);
};


#endif // _ONI_DRIVER_TYPES_H_
