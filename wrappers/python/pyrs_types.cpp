/* License: Apache 2.0. See LICENSE file in root directory.
Copyright(c) 2017 Intel Corporation. All Rights Reserved. */

#include "pyrealsense2.h"
#include <librealsense2/hpp/rs_types.hpp>

void init_types(py::module &m) {
    /** rs2_types.hpp **/
    // error+subclasses
    enum product_line
    {
        any = RS2_PRODUCT_LINE_ANY,
        any_intel = RS2_PRODUCT_LINE_ANY_INTEL,
        non_intel = RS2_PRODUCT_LINE_NON_INTEL,
        D400 = RS2_PRODUCT_LINE_D400,
        D500 = RS2_PRODUCT_LINE_D500,
        SR300 = RS2_PRODUCT_LINE_SR300,
        L500 = RS2_PRODUCT_LINE_L500,
        T200 = RS2_PRODUCT_LINE_T200,
        DEPTH = RS2_PRODUCT_LINE_DEPTH,
        TRACKING = RS2_PRODUCT_LINE_TRACKING,
        sw_only = RS2_PRODUCT_LINE_SW_ONLY
    };

    py::enum_< product_line >( m, "product_line" )
        .value( "any", product_line::any )
        .value( "any_intel", product_line::any_intel )
        .value( "non_intel", product_line::non_intel )
        .value( "sw_only", product_line::sw_only )
        .value( "D400", product_line::D400 )
        .value( "D500", product_line::D500 )
        .value( "SR300", product_line::SR300 )
        .value( "L500", product_line::L500 )
        .value( "T200", product_line::T200 )
        .value( "DEPTH", product_line::DEPTH )
        .value( "TRACKING", product_line::TRACKING )
        .export_values();

    py::class_<rs2::option_range> option_range(m, "option_range"); // No docstring in C++
    option_range.def(py::init<>())
        .def( py::init(
                  []( float min, float max, float step, float default_value ) {
                      return rs2::option_range{ min, max, step, default_value };
                  } ),
              "min"_a,
              "max"_a,
              "step"_a,
              "default"_a )
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

    BIND_ENUM(m, rs2_calib_location, RS2_CALIB_LOCATION_COUNT, "Calib Location")

    py::class_<rs2_calibration_roi> calibration_roi(m, "calibration_roi"); // No docstring in C++
    calibration_roi.def(py::init<>())
        .def(py::init<uint16_t, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t>())
        .def_property(BIND_RAW_2D_ARRAY_PROPERTY(rs2_calibration_roi, mask_pixel, uint16_t, 4, 2), 
            "Array of four corners in Deph Frame Coordinate system that define a closed simple quadrangle");

    py::class_<float3_row_major> float3(m, "float3"); // No docstring in C++
    float3.def(py::init<>())
        .def(py::init<float, float, float>())
        .def_readwrite("x", &float3_row_major::x, "x")
        .def_readwrite("y", &float3_row_major::y, "y")
        .def_readwrite("z", &float3_row_major::z, "z");

    py::class_<float3x3_row_major> float3x3(m, "float3x3"); // No docstring in C++
    float3x3.def(py::init<>())
        .def(py::init<float3_row_major, float3_row_major, float3_row_major>())
        .def_readwrite("x", &float3x3_row_major::x, "x")
        .def_readwrite("y", &float3x3_row_major::y, "y")
        .def_readwrite("z", &float3x3_row_major::z, "z");

    py::class_<rs2_extrinsics_row_major> extrinsics_table(m, "extrinsics_table"); // No docstring in C++
    extrinsics_table.def(py::init<>())
        .def(py::init<float3x3_row_major, float3_row_major>())
        .def_readwrite("rotation", &rs2_extrinsics_row_major::rotation, "Rotation Value")
        .def_readwrite("translation", &rs2_extrinsics_row_major::translation, "Translation Value");

    py::class_<rs2_calibration_config> calibration_config(m, "calibration_config"); // No docstring in C++
    calibration_config.def(py::init<>())
        .def_readwrite("calib_roi_num_of_segments", &rs2_calibration_config::calib_roi_num_of_segments, "calib_roi_num_of_segments")
        .def_property(BIND_RAW_ARRAY_PROPERTY(rs2_calibration_config, roi, rs2_calibration_roi, 4), "roi")
        .def_property(BIND_RAW_ARRAY_PROPERTY(rs2_calibration_config, reserved1, uint8_t, sizeof(rs2_calibration_config::reserved1)), "reserved1")
        .def_readwrite("camera_position", &rs2_calibration_config::camera_position, "camera_position")
        .def_property(BIND_RAW_ARRAY_PROPERTY(rs2_calibration_config, reserved2, uint8_t, sizeof(rs2_calibration_config::reserved2)), "reserved2")
        .def_property(BIND_RAW_ARRAY_PROPERTY(rs2_calibration_config, crypto_signature, uint8_t, sizeof(rs2_calibration_config::crypto_signature)), "crypto_signature")
        .def_property(BIND_RAW_ARRAY_PROPERTY(rs2_calibration_config, reserved3, uint8_t, sizeof(rs2_calibration_config::reserved3)), "reserved3")
        .def("__eq__", [](const rs2_calibration_config& self, const rs2_calibration_config& other) {
            return self == other;
        });

    /** end rs_types.hpp **/
}

