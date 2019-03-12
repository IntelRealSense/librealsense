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
#ifndef _ONI_PLATFORM_WIN32_H_
#define _ONI_PLATFORM_WIN32_H_

//---------------------------------------------------------------------------
// Prerequisites
//---------------------------------------------------------------------------
#ifndef WINVER						// Allow use of features specific to Windows XP or later
	#define WINVER 0x0501
#endif
#ifndef _WIN32_WINNT				// Allow use of features specific to Windows XP or later
	#define _WIN32_WINNT 0x0501
#endif						
#ifndef _WIN32_WINDOWS				// Allow use of features specific to Windows 98 or later
	#define _WIN32_WINDOWS 0x0410
#endif
#ifndef _WIN32_IE					// Allow use of features specific to IE 6.0 or later
	#define _WIN32_IE 0x0600
#endif
#define WIN32_LEAN_AND_MEAN			// Exclude rarely-used stuff from Windows headers

// Undeprecate CRT functions
#ifndef _CRT_SECURE_NO_DEPRECATE 
	#define _CRT_SECURE_NO_DEPRECATE 1
#endif

//---------------------------------------------------------------------------
// Includes
//---------------------------------------------------------------------------
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <io.h>
#include <time.h>
#include <assert.h>
#include <float.h>
#include <crtdbg.h>

#if _MSC_VER < 1600 // Visual Studio 2008 and older doesn't have stdint.h...
typedef signed char int8_t;
typedef short int16_t;
typedef int int32_t;
typedef __int64 int64_t;

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned __int64 uint64_t;
#else
#include <stdint.h>
#endif

//---------------------------------------------------------------------------
// Platform Basic Definition
//---------------------------------------------------------------------------
#define ONI_PLATFORM ONI_PLATFORM_WIN32
#define ONI_PLATFORM_STRING "Win32"

//---------------------------------------------------------------------------
// Platform Capabilities
//---------------------------------------------------------------------------
#define ONI_PLATFORM_ENDIAN_TYPE ONI_PLATFORM_IS_LITTLE_ENDIAN

#define ONI_PLATFORM_SUPPORTS_DYNAMIC_LIBS 1

//---------------------------------------------------------------------------
// Memory
//---------------------------------------------------------------------------
/** The default memory alignment. */ 
#define ONI_DEFAULT_MEM_ALIGN 16

/** The thread static declarator (using TLS). */
#define ONI_THREAD_STATIC __declspec(thread)

//---------------------------------------------------------------------------
// Files
//---------------------------------------------------------------------------
/** The maximum allowed file path size (in bytes). */ 
#define ONI_FILE_MAX_PATH MAX_PATH

//---------------------------------------------------------------------------
// Call backs
//---------------------------------------------------------------------------
/** The std call type. */ 
#define ONI_STDCALL __stdcall

/** The call back calling convention. */ 
#define ONI_CALLBACK_TYPE ONI_STDCALL

/** The C and C++ calling convension. */
#define ONI_C_DECL __cdecl

//---------------------------------------------------------------------------
// Macros
//---------------------------------------------------------------------------
/** Returns the date and time at compile time. */ 
#define ONI_TIMESTAMP __DATE__ " " __TIME__

/** Converts n into a pre-processor string.  */ 
#define ONI_STRINGIFY(n) ONI_STRINGIFY_HELPER(n)
#define ONI_STRINGIFY_HELPER(n) #n

//---------------------------------------------------------------------------
// API Export/Import Macros
//---------------------------------------------------------------------------
/** Indicates an exported shared library function. */ 
#define ONI_API_EXPORT __declspec(dllexport)

/** Indicates an imported shared library function. */ 
#define ONI_API_IMPORT __declspec(dllimport)

/** Indicates a deprecated function */
#if _MSC_VER < 1400 // Before VS2005 there was no support for declspec deprecated...
	#define ONI_API_DEPRECATED(msg)
#else
	#define ONI_API_DEPRECATED(msg) __declspec(deprecated(msg))
#endif

#endif //_ONI_PLATFORM_WIN32_H_
