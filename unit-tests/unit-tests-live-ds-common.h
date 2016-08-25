// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

/////////////////////////////////////////////////////////
// This set of tests is valid only for the DS-device camera //
/////////////////////////////////////////////////////////
#pragma once
#ifndef LIBREALSENSE_UNITTESTS_LIVE_DS_COMMON_H
#define LIBREALSENSE_UNITTESTS_LIVE_DS_COMMON_H

#include "catch/catch.hpp"

#include "unit-tests-common.h"

#include <algorithm>
#include <iostream>
#include <limits>

// noexcept is not accepted by Visual Studio 2013 yet, but noexcept(false) is require on throwing destructors on gcc and clang
// It is normally advisable not to throw in a destructor, however, this usage is safe for require_error/require_no_error because
// they will only ever be created as temporaries immediately before being passed to a C ABI function. All parameters and return
// types are vanilla C types, and thus nothrow-copyable, and the function itself cannot throw because it is a C ABI function.
// Therefore, when a temporary require_error/require_no_error is destructed immediately following one of these C ABI function
// calls, we should not have any exceptions in flight, and can freely throw (perhaps indirectly by calling Catch's REQUIRE() 
// macro) to indicate postcondition violations.
#ifdef WIN32
#define NOEXCEPT_FALSE
#else
#define NOEXCEPT_FALSE noexcept(false)
#endif

#include "unit-tests-common.h"

// TODO consider to expose cameras id strings in rs.hpp via <PID:NAME> construct
static const std::vector<std::string> ds_names = { "Intel RealSense R200" ,"Intel RealSense LR200", "Intel RealSense ZR300" };
enum { Intel_R200, Intel_LR200, Intel_ZR300};

enum { BEFORE_START_DEVICE = 1, AFTER_START_DEVICE = 2 };

inline void test_ds_device_option(rs_option option, std::initializer_list<int> values, std::initializer_list<int> bad_values, int when)
{
    safe_context ctx;
    REQUIRE(rs_get_device_count(ctx, require_no_error()) == 1);

    rs_device * dev = rs_get_device(ctx, 0, require_no_error());
    REQUIRE(dev != nullptr);
    REQUIRE(std::any_of(ds_names.begin(), ds_names.end(), [&](std::string const& s) {return s == rs_get_device_name(dev, require_no_error()); }));

    if (when & BEFORE_START_DEVICE)
    {
        test_option(dev, option, values, bad_values);
    }

    if (when & AFTER_START_DEVICE)
    {
        rs_enable_stream_preset(dev, RS_STREAM_DEPTH, RS_PRESET_BEST_QUALITY, require_no_error());
        rs_start_device(dev, require_no_error());

        // Currently, setting/getting options immediately after streaming frequently raises hardware errors
        // todo - Internally block or retry failed calls within the first few seconds after streaming
        std::this_thread::sleep_for(std::chrono::seconds(1));
        test_option(dev, option, values, bad_values);
    }
}


inline void test_ds_device_streaming(std::initializer_list<stream_mode> modes)
{
    safe_context ctx;
    REQUIRE(rs_get_device_count(ctx, require_no_error()) == 1);

    rs_device * dev = rs_get_device(ctx, 0, require_no_error());
    REQUIRE(dev != nullptr);
    REQUIRE(std::any_of(ds_names.begin(), ds_names.end(), [&](std::string const& s) {return s == rs_get_device_name(dev, require_no_error()); }));

    test_streaming(dev, modes);
}


#endif
