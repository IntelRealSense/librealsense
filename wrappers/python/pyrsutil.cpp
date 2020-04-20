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
    }, "Given a point in 3D space, compute the corresponding pixel coordinates in an image with no distortion or forward distortion coefficients produced by the same camera",
       "intrin"_a, "point"_a);

    m.def("rs2_deproject_pixel_to_point", [](const rs2_intrinsics& intrin, const std::array<float, 2>& pixel, float depth)->std::array<float, 3>
    {
        std::array<float, 3> point{};
        rs2_deproject_pixel_to_point(point.data(), &intrin, pixel.data(), depth);
        return point;
    }, "Given pixel coordinates and depth in an image with no distortion or inverse distortion coefficients, compute the corresponding point in 3D space relative to the same camera",
       "intrin"_a, "pixel"_a, "depth"_a);

    m.def("rs2_transform_point_to_point", [](const rs2_extrinsics& extrin, const std::array<float, 3>& from_point)->std::array<float, 3>
    {
        std::array<float, 3> to_point{};
        rs2_transform_point_to_point(to_point.data(), &extrin, from_point.data());
        return to_point;
    }, "Transform 3D coordinates relative to one sensor to 3D coordinates relative to another viewpoint",
       "extrin"_a, "from_point"_a);

    m.def("rs2_fov", [](const rs2_intrinsics& intrin)->std::array<float, 2>
    {
        std::array<float, 2> to_fow{};
        rs2_fov(&intrin, to_fow.data());
        return to_fow;
    }, "Calculate horizontal and vertical field of view, based on video intrinsics", "intrin"_a);

    m.def("next_pixel_in_line", [](std::array<float, 2> curr, const std::array<float, 2> start, const std::array<float, 2> end)->std::array<float, 2>
    {
        next_pixel_in_line(curr.data(), start.data(), end.data());
        return curr;
    }, "curr"_a, "start"_a, "end"_a);

    m.def("is_pixel_in_line", [](std::array<float, 2> curr, const std::array<float, 2> start, const std::array<float, 2> end)->bool
    {
        return is_pixel_in_line(curr.data(), start.data(), end.data());
    }, "curr"_a, "start"_a, "end"_a); // Wrapping needed because raw arrays

    m.def("adjust_2D_point_to_boundary", [](std::array<float, 2> p, int width, int height)->std::array<float, 2>
    {
        adjust_2D_point_to_boundary(p.data(), width, height);
        return p;
    }, "p"_a, "width"_a, "height"_a);

    auto cp_to_dp = [](BufData data, float depth_scale, float depth_min, float depth_max,
            const rs2_intrinsics& depth_intrin, const rs2_intrinsics& color_intrin,
            const rs2_extrinsics& color_to_depth, const rs2_extrinsics& depth_to_color,
            std::array<float, 2> from_pixel)->std::array<float, 2>
    {
        std::array<float, 2> to_pixel;
        rs2_project_color_pixel_to_depth_pixel(to_pixel.data(), static_cast<const uint16_t*>(data._ptr), 
                depth_scale, depth_min, depth_max, &depth_intrin, &color_intrin, &depth_to_color,
                &color_to_depth, from_pixel.data());
        return to_pixel;
    };

    m.def("rs2_project_color_pixel_to_depth_pixel", cp_to_dp, "data"_a, "depth_scale"_a,
          "depth_min"_a, "depth_max"_a, "depth_intrin"_a, "color_intrin"_a, "depth_to_color"_a,
          "color_to_depth"_a, "from_pixel"_a);
    /** end rsutil.h **/
}
