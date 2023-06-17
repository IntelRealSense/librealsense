/* License: Apache 2.0. See LICENSE file in root directory.
Copyright(c) 2017 Intel Corporation. All Rights Reserved. */

#include "python.hpp"
#include "../include/librealsense2/hpp/rs_types.hpp"

void init_types(py::module &m) {
    /** rs2_types.hpp **/
    // error+subclasses
    enum product_line
    {
        any = RS2_PRODUCT_LINE_ANY,
        any_intel = RS2_PRODUCT_LINE_ANY_INTEL,
        non_intel = RS2_PRODUCT_LINE_NON_INTEL,
        D400 = RS2_PRODUCT_LINE_D400,
        SR300 = RS2_PRODUCT_LINE_SR300,
        L500 = RS2_PRODUCT_LINE_L500,
        T200 = RS2_PRODUCT_LINE_T200,
        DEPTH = RS2_PRODUCT_LINE_DEPTH,
        TRACKING = RS2_PRODUCT_LINE_TRACKING
    };

    py::enum_< product_line >( m, "product_line" )
        .value( "any", product_line::any )
        .value( "any_intel", product_line::any_intel )
        .value( "non_intel", product_line::non_intel )
        .value( "D400", product_line::D400 )
        .value( "SR300", product_line::SR300 )
        .value( "L500", product_line::L500 )
        .value( "T200", product_line::T200 )
        .value( "DEPTH", product_line::DEPTH )
        .value( "TRACKING", product_line::TRACKING )
        .export_values();

    py::class_<rs2::option_range> option_range(m, "option_range"); // No docstring in C++
    option_range.def(py::init<>())
        .def_readwrite("min", &rs2::option_range::min)
        .def_readwrite("max", &rs2::option_range::max)
        .def_readwrite("default", &rs2::option_range::def)
        .def_readwrite("step", &rs2::option_range::step)
        .def("__repr__", [](const rs2::option_range &self) {
            std::stringstream ss;
            ss << "<" SNAME ".option_range: " << self.min << "-" << self.max
                << "/" << self.step << " [" << self.def << "]>";
            return ss.str();
        });

    py::class_<rs2::region_of_interest> region_of_interest(m, "region_of_interest"); // No docstring in C++
    region_of_interest.def(py::init<>())
        .def_readwrite("min_x", &rs2::region_of_interest::min_x)
        .def_readwrite("min_y", &rs2::region_of_interest::min_y)
        .def_readwrite("max_x", &rs2::region_of_interest::max_x)
        .def_readwrite("max_y", &rs2::region_of_interest::max_y);
    /** end rs_types.hpp **/
}
