/*****************************************************************************
*                                                                            *
*  PrimeSense PSCommon Library                                               *
*  Copyright (C) 2012 PrimeSense Ltd.                                        *
*                                                                            *
*  This file is part of PSCommon.                                            *
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
#ifndef _ONI_PLATFORM_MACOSX_H_
#define _ONI_PLATFORM_MACOSX_H_

// Start with Linux-x86, and override what's different
#include "../Linux-x86/OniPlatformLinux-x86.h"

#include <sys/time.h>

#undef ONI_PLATFORM
#undef ONI_PLATFORM_STRING
#define ONI_PLATFORM ONI_PLATFORM_MACOSX
#define ONI_PLATFORM_STRING "MacOSX"

#define ONI_PLATFORM_HAS_NO_TIMED_OPS
#define ONI_PLATFORM_HAS_NO_CLOCK_GETTIME
#define ONI_PLATFORM_HAS_NO_SCHED_PARAM
#define ONI_PLATFORM_HAS_BUILTIN_SEMUN

#undef ONI_THREAD_STATIC
#define ONI_THREAD_STATIC 
 
#endif //_ONI_PLATFORM_MACOSX_H_
