// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "types.h"

#include "core/notification.h"
#include <librealsense2/hpp/rs_processing.hpp>

#include <algorithm>
#include <iomanip>
#include <numeric>
#include <fstream>
#include <cmath>

const double SQRT_DBL_EPSILON = sqrt(std::numeric_limits<double>::epsilon());

namespace librealsense
{
    // The extrinsics on the camera ("raw extrinsics") are in milimeters, but LRS works in meters
    // Additionally, LRS internal algorithms are
    // written with a transposed matrix in mind! (see rs2_transform_point_to_point)
    rs2_extrinsics to_raw_extrinsics(rs2_extrinsics extr)
    {
        float const meters_to_milimeters = 1000;
        extr.translation[0] *= meters_to_milimeters;
        extr.translation[1] *= meters_to_milimeters;
        extr.translation[2] *= meters_to_milimeters;

        std::swap(extr.rotation[1], extr.rotation[3]);
        std::swap(extr.rotation[2], extr.rotation[6]);
        std::swap(extr.rotation[5], extr.rotation[7]);
        return extr;
    }

    rs2_extrinsics from_raw_extrinsics(rs2_extrinsics extr)
    {
        float const milimeters_to_meters = 0.001f;
        extr.translation[0] *= milimeters_to_meters;
        extr.translation[1] *= milimeters_to_meters;
        extr.translation[2] *= milimeters_to_meters;

        std::swap(extr.rotation[1], extr.rotation[3]);
        std::swap(extr.rotation[2], extr.rotation[6]);
        std::swap(extr.rotation[5], extr.rotation[7]);
        return extr;
    }

    std::string make_less_screamy(const char* str)
    {
        std::string res(str);

        bool first = true;
        for (auto i = 0; i < res.size(); i++)
        {
            if (res[i] != '_')
            {
                if (!first) res[i] = tolower(res[i]);
                first = false;
            }
            else
            {
                res[i] = ' ';
                first = true;
            }
        }

        return res;
    }

    recoverable_exception::recoverable_exception(const std::string& msg,
        rs2_exception_type exception_type) noexcept
        : librealsense_exception(msg, exception_type)
    {
        LOG_DEBUG("recoverable_exception: " << msg);
    }

    bool file_exists(const char* filename)
    {
        std::ifstream f(filename);
        return f.good();
    }

}
