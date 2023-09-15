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
#ifndef _ONI_PLATFORM_LINUX_X86_H_
#define _ONI_PLATFORM_LINUX_X86_H_

//---------------------------------------------------------------------------
// Prerequisites
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Includes
//---------------------------------------------------------------------------
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <stdint.h>

//---------------------------------------------------------------------------
// Platform Basic Definition
//---------------------------------------------------------------------------
#define ONI_PLATFORM ONI_PLATFORM_LINUX_X86
#define ONI_PLATFORM_STRING "Linux-x86"

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
#define ONI_THREAD_STATIC __thread

//---------------------------------------------------------------------------
// Files
//---------------------------------------------------------------------------
/** The maximum allowed file path size (in bytes). */ 
#define ONI_FILE_MAX_PATH 256

//---------------------------------------------------------------------------
// Call back
//---------------------------------------------------------------------------
/** The std call type. */ 
#define ONI_STDCALL __stdcall

/** The call back calling convention. */ 
#define ONI_CALLBACK_TYPE 

/** The C and C++ calling convension. */
#define ONI_C_DECL

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
#define ONI_API_EXPORT __attribute__ ((visibility("default")))

/** Indicates an imported shared library function. */ 
#define ONI_API_IMPORT 

/** Indicates a deprecated function */
#define ONI_API_DEPRECATED(msg) __attribute__((warning("This function is deprecated: " msg)))

#endif //_ONI_PLATFORM_LINUX_X86_H_

