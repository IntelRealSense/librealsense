/* License: Apache 2.0. See LICENSE file in root directory.
Copyright(c) 2017 Intel Corporation. All Rights Reserved. */

#include "python.hpp"
#include "../include/librealsense2/hpp/rs_export.hpp"

void init_export(py::module &m) {
    /* rs_export.hpp */
    // py::class_<rs2::save_to_ply, rs2::filter> save_to_ply(m, "save_to_ply"); // No docstring in C++
    // save_to_ply.def(py::init<std::string, rs2::pointcloud>(), "filename"_a = "RealSense Pointcloud ", "pc"_a = rs2::pointcloud())
    //     .def_readonly_static("option_ignore_color", &rs2::save_to_ply::OPTION_IGNORE_COLOR);

    py::class_<rs2::save_single_frameset, rs2::filter> save_single_frameset(m, "save_single_frameset"); // No docstring in C++
    save_single_frameset.def(py::init<std::string>(), "filename"_a = "RealSense Frameset ");
    /** end rs_export.hpp **/
}
