// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "catch/catch.hpp"
#include <cmath>
#include <iostream>
#include <chrono>
#include <ctime>
#include <algorithm>
#include <type_traits>
#include <librealsense2/rs.hpp>
#include <librealsense2/hpp/rs_sensor.hpp>
#include "../../common/tiny-profiler.h"
#include "./../src/environment.h"

using namespace librealsense;
using namespace librealsense::platform;

TEST_CASE("copy_array", "[code]")
{
    size_t elem = 0;
    float src_float[]   = { 1.1f, 2.2f, 3.3f, -5.5f, 0.f, 123534.f };
    double src_double[] = { 0.00000000000000000001, -2222222222222222222222222.222222, 334529087543.30875246784, -5.51234533524532345254645234256, 5888985940.4535, -0.0000000000001 };

    constexpr size_t src_size = arr_size(src_float);

    float       tgt_float[src_size]     = { 0 };
    double      tgt_double[src_size]    = { 0 };

    // Check that precision is preserved when using identical or a larger data type
    elem = librealsense::copy_array(tgt_float, src_float);
    REQUIRE(elem == src_size);
    elem = librealsense::copy_array(tgt_double, src_float);
    REQUIRE(elem == src_size);

    // Check that precision is preserved when expanding o using similar data types
    for (size_t i = 0; i < src_size; ++i)
    {
        CAPTURE(src_double[i]);
        CAPTURE(tgt_float[i]);
        CAPTURE(tgt_double[i]);
        REQUIRE(src_float[i] == (tgt_float[i]));
        REQUIRE(src_float[i] == (tgt_double[i]));
    }

    // Check that precision is lost when narrowing data type
    memset(&tgt_float, 0, sizeof(float)*src_size);
    elem = librealsense::copy_array<true>(tgt_float, src_double);
    REQUIRE(elem == src_size);

    for (size_t i = 0; i < src_size; ++i)
    {
        CAPTURE(src_double[i]);
        CAPTURE(tgt_float[i]);
        REQUIRE(src_double[i] != tgt_float[i]);
    }
}

TEST_CASE("copy_2darray", "[code]")
{
    size_t elem = 0;
    float src_float[2][6] = { { 1.1f, 2.2f, 3.3f, -5.5f, 0.f, 123534.f },
                                { 764.07f, -2342.2f, 2453.653f, -445.215f, 0.11324f, 1224.8209f } };
    double src_double[2][6] = { { -254354352341.1, 765732052.21124, 3.4432007654764633233554, -524432.5432650645, 0.0000000000000000112, 123534.4234254673 },
                                { 764.07654343263, -242675463465342.243, -9876322453.6342453, -42315432545.2153542, 0.1345521543251324, -0.0000123413242329 } };

    constexpr size_t src_size = arr_size(src_float);

    // Get the internal dimension sizes
    auto h = std::extent<decltype(src_float),0>::value;
    auto w = std::extent<decltype(src_float),1>::value;
    
    float       tgt_float[2][6] = { 0 };
    double      tgt_double[2][6] = { 0 };

    // Check that precision is preserved when using identical or a larger data type
    elem = librealsense::copy_2darray(tgt_float, src_float);
    REQUIRE(elem == src_size);
    elem = librealsense::copy_2darray(tgt_double, src_float);
    REQUIRE(elem == src_size);

    for (size_t i = 0; i < h; ++i)
        for (size_t j = 0; j < w; ++j)
        {
            CAPTURE(src_float[i][j]);
            CAPTURE(tgt_float[i][j]);
            CAPTURE(tgt_double[i][j]);
            REQUIRE(src_float[i][j] == (tgt_float[i][j]));
            REQUIRE(src_float[i][j] == (tgt_double[i][j]));
        }

    // Check that precision is lost when narrowing data type
    memset(&tgt_float, 0, sizeof(float)*src_size);
    elem = librealsense::copy_2darray<true>(tgt_float, src_double);
    REQUIRE(elem == src_size);

    for (size_t i = 0; i < h; ++i)
        for (size_t j = 0; j < w; ++j)
        {
            CAPTURE(src_double[i][j]);
            CAPTURE(tgt_float[i][j]);
            REQUIRE(src_double[i][j] != tgt_float[i][j]);
        }
}
