/* License: Apache 2.0. See LICENSE file in root directory.
Copyright(c) 2017 Intel Corporation. All Rights Reserved. */

#include "python.hpp"
#include "../include/librealsense2/rs.hpp"
#include "../include/librealsense2/hpp/rs_export.hpp"

PYBIND11_MODULE(NAME, m) {
    m.doc() = R"pbdoc(
        LibrealsenseTM Python Bindings
        ==============================
        Library for accessing Intel RealSenseTM cameras
    )pbdoc";
    m.attr("__version__") = RS2_API_VERSION_STR;

    init_c_files(m);
    init_types(m);
    init_frame(m);
    init_options(m);
    init_processing(m);
    init_sensor(m);
    init_device(m);
    init_record_playback(m);
    init_context(m);
    init_pipeline(m);
    init_internal(m); // must be run after init_frame()
    init_export(m);
    init_advanced_mode(m);
    init_util(m);
    
    /** rs_export.hpp **/
    py::class_<rs2::save_to_ply, rs2::filter>(m, "save_to_ply")
        .def(py::init<std::string, rs2::pointcloud>(), "filename"_a = "RealSense Pointcloud ", "pc"_a = rs2::pointcloud())
        .def_property_readonly_static("option_ignore_color", [](py::object) { return rs2::save_to_ply::OPTION_IGNORE_COLOR; })
        .def_property_readonly_static("option_ply_mesh", [](py::object) { return rs2::save_to_ply::OPTION_PLY_MESH; })
        .def_property_readonly_static("option_ply_binary", [](py::object) { return rs2::save_to_ply::OPTION_PLY_BINARY; })
        .def_property_readonly_static("option_ply_normals", [](py::object) { return rs2::save_to_ply::OPTION_PLY_NORMALS; })
        .def_property_readonly_static("option_ply_threshold", [](py::object) { return rs2::save_to_ply::OPTION_PLY_THRESHOLD; });

    /** rs.hpp **/
    m.def("log_to_console", &rs2::log_to_console, "min_severity"_a);
    m.def("log_to_file", &rs2::log_to_file, "min_severity"_a, "file_path"_a);

    py::class_<rs2::log_message> log_message(m, "log_message");
    log_message.def("line_number", &rs2::log_message::line_number)
        .def("filename", &rs2::log_message::filename)
        .def("raw", &rs2::log_message::raw)
        .def("full", &rs2::log_message::full)
        .def("__str__", &rs2::log_message::raw)
        .def("__repr__", &rs2::log_message::full);

    m.def("log_to_callback", [](rs2_log_severity min_severity, std::function<void(rs2_log_severity, rs2::log_message)> callback)
    {
        rs2::log_to_callback(min_severity, callback);
    }, "min_severity"_a, "callback"_a);
    m.def("log", &rs2::log, "severity"_a, "message"_a);

    // rs2::log?
    /** end rs.hpp **/
}
