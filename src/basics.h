// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#pragma once


// Disable declspec(dllexport) warnings:
// Classes exported via LRS_EXTENSION_API are **not** part of official librealsense API (at least for now)
// Any extension relying on these APIs must be compiled and distributed together with realsense2.dll
#pragma warning(disable : 4275)        /* disable: C4275: non dll-interface class used as base for dll-interface class */
#pragma warning(disable : 4251)        /* disable: C4251: class needs to have dll-interface to be used by clients of class */

// Compiler Warning (level 2) C4250: 'class1' : inherits 'class2::member' via dominance
// This happens for librealsense::device, basically warning us of something that we know and is OK:
//     'librealsense::device': inherits 'info_container::create_snapshot' via dominance (and repeats for 3 more functions)
// And for anything else using both an interface and a container, options_container etc.
#pragma warning(disable: 4250)


#ifdef WIN32
#define LRS_EXTENSION_API __declspec(dllexport)
#else
#define LRS_EXTENSION_API
#endif
