/* License: Apache 2.0. See LICENSE file in root directory.
Copyright(c) 2017 Intel Corporation. All Rights Reserved. */

#include "python.hpp"
#include "../include/librealsense2/rsutil.h"

void init_util(py::module &m) {
    /** rsutil.h **/
    m.def("rs2_project_point_to_pixel", [](const rs2_intrinsics& intrin, const std::array<float, 3>& point)->std::array<float, 2>
    {
        std::array<float, 2> pixel{};
        rs2_project_point_to_pixel(pixel.data(), &intrin, point.data());
        return pixel;
    }, "Given a point in 3D space, compute the corresponding pixel coordinates in an image with no distortion or forward distortion coefficients produced by the same camera");

    m.def("rs2_deproject_pixel_to_point", [](const rs2_intrinsics& intrin, const std::array<float, 2>& pixel, float depth)->std::array<float, 3>
    {
        std::array<float, 3> point{};
        rs2_deproject_pixel_to_point(point.data(), &intrin, pixel.data(), depth);
        return point;
    }, "Given pixel coordinates and depth in an image with no distortion or inverse distortion coefficients, compute the corresponding point in 3D space relative to the same camera");

    m.def("rs2_transform_point_to_point", [](const rs2_extrinsics& extrin, const std::array<float, 3>& from_point)->std::array<float, 3>
    {
        std::array<float, 3> to_point{};
        rs2_transform_point_to_point(to_point.data(), &extrin, from_point.data());
        return to_point;
    }, "Transform 3D coordinates relative to one sensor to 3D coordinates relative to another viewpoint");

    m.def("rs2_fov", [](const rs2_intrinsics& intrin)->std::array<float, 2>
    {
        std::array<float, 2> to_fow{};
        rs2_fov(&intrin, to_fow.data());
        return to_fow;
    }, "Calculate horizontal and vertical field of view, based on video intrinsics");
    /** end rsutil.h **/
}
