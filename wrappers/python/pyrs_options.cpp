/* License: Apache 2.0. See LICENSE file in root directory.
Copyright(c) 2017 Intel Corporation. All Rights Reserved. */

#include "python.hpp"
#include "../include/librealsense2/hpp/rs_options.hpp"

void init_options(py::module &m) {
    /** rs_options.hpp **/
    py::class_<rs2::options> options(m, "options", "Base class for options interface. Should be used via sensor or processing_block."); // No docstring in C++
    options.def("is_option_read_only", &rs2::options::is_option_read_only, "Check if particular option "
                "is read only.", "option"_a)
        .def("get_option", &rs2::options::get_option, "Read option value from the device.", "option"_a)
        .def("get_option_range", &rs2::options::get_option_range, "Retrieve the available range of values "
             "of a supported option", "option"_a)
        .def("set_option", &rs2::options::set_option, "Write new value to device option", "option"_a, "value"_a)
        .def("supports", (bool (rs2::options::*)(rs2_option option) const) &rs2::options::supports, "Check if particular "
             "option is supported by a subdevice", "option"_a)
        .def("get_option_description", &rs2::options::get_option_description, "Get option description.", "option"_a)
        .def("get_option_value_description", &rs2::options::get_option_value_description, "Get option value description "
             "(In case a specific option value holds special meaning)", "option"_a, "value"_a)
        .def("get_supported_options", &rs2::options::get_supported_options, "Retrieve list of supported options"); // No docstring in C++
    /** end rs_options.hpp **/
}
