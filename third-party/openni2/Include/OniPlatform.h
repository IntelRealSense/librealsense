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
#ifndef _ONI_PLATFORM_H_
#define _ONI_PLATFORM_H_

// Supported platforms
#define ONI_PLATFORM_WIN32 1
#define ONI_PLATFORM_LINUX_X86 2
#define ONI_PLATFORM_LINUX_ARM 3
#define ONI_PLATFORM_MACOSX 4
#define ONI_PLATFORM_ANDROID_ARM 5

#if (defined _WIN32)
#	ifndef RC_INVOKED
#		if _MSC_VER < 1300
#			error OpenNI Platform Abstraction Layer - Win32 - Microsoft Visual Studio version below 2003 (7.0) are not supported!
#		endif
#	endif
#	include "Win32/OniPlatformWin32.h"
#elif defined (ANDROID) && defined (__arm__)
#	include "Android-Arm/OniPlatformAndroid-Arm.h"
#elif (__linux__ && (i386 || __x86_64__))
#	include "Linux-x86/OniPlatformLinux-x86.h"
#elif (__linux__ && __arm__)
#	include "Linux-Arm/OniPlatformLinux-Arm.h"
#elif _ARC
#	include "ARC/OniPlaformARC.h"
#elif (__APPLE__)
#	include "MacOSX/OniPlatformMacOSX.h"
#else
#	error Xiron Platform Abstraction Layer - Unsupported Platform!
#endif

#ifdef __cplusplus
#	define ONI_C extern "C"
#	define ONI_C_API_EXPORT ONI_C ONI_API_EXPORT
#	define ONI_C_API_IMPORT ONI_C ONI_API_IMPORT
#	define ONI_CPP_API_EXPORT ONI_API_EXPORT
#	define ONI_CPP_API_IMPORT ONI_API_IMPORT
#else // __cplusplus
#	define ONI_C_API_EXPORT ONI_API_EXPORT
#	define ONI_C_API_IMPORT ONI_API_IMPORT
#endif  // __cplusplus

#ifdef OPENNI2_EXPORT
#	define ONI_C_API ONI_C_API_EXPORT
#	define ONI_CPP_API ONI_CPP_API_EXPORT
#else // OPENNI2_EXPORT
#	define ONI_C_API ONI_C_API_IMPORT
#	define ONI_CPP_API ONI_CPP_API_IMPORT
#endif // OPENNI2_EXPORT


#endif // _ONI_PLATFORM_H_
