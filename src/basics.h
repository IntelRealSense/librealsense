// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#pragma once


// Disable declspec(dllexport) warnings:
// Classes exported via LRS_EXTENSION_API are **not** part of official librealsense API (at least for now)
// Any extension relying on these APIs must be compiled and distributed together with realsense2.dll
#pragma warning(disable : 4275)        /* disable: C4275: non dll-interface class used as base for dll-interface class */
#pragma warning(disable : 4251)        /* disable: C4251: class needs to have dll-interface to be used by clients of class */
#ifdef WIN32
#define LRS_EXTENSION_API __declspec(dllexport)
#else
#define LRS_EXTENSION_API
#endif
