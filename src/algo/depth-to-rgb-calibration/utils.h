// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#define PI 3.14159265359

namespace librealsense {
namespace algo {
namespace depth_to_rgb_calibration {

    void inv(const double x[9], double y[9]); //generated code
    void transpose(const double x[9], double y[9]);
    void rotate_180(const uint8_t* A, uint8_t* B, uint32_t w, uint32_t h); //generated code
    const std::vector<double> interp1(const std::vector<double> ind, const std::vector<double> vals, const std::vector<double> intrp);
}
}
}

