/* License: Apache 2.0. See LICENSE file in root directory.
Copyright(c) 2017 Intel Corporation. All Rights Reserved. */

#include "pyrealsense2.h"
#include <librealsense2/rs.h>
#include <iomanip>
#include <src/types.h>
#include "../include/librealsense2/hpp/rs_safety_types.hpp"


void init_safety(py::module &m) {
   
    /** rs_safety_types.h **/
    py::class_<sc_float2> float2(m, "float2"); // No docstring in C++
    float2.def(py::init<>())
        .def(py::init<float, float>())
        .def_readwrite("x", &sc_float2::x, "x")
        .def_readwrite("y", &sc_float2::y, "y");

    py::class_<sc_float3> float3(m, "float3"); // No docstring in C++
    float3.def(py::init<>())
        .def(py::init<float, float, float>())
        .def_readwrite("x", &sc_float3::x, "x")
        .def_readwrite("y", &sc_float3::y, "y")
        .def_readwrite("z", &sc_float3::z, "z");

    py::class_<sc_float3x3> float3x3(m, "float3x3"); // No docstring in C++
    float3x3.def(py::init<>())
        .def(py::init<sc_float3, sc_float3, sc_float3>())
        .def_readwrite("x", &sc_float3x3::x, "x")
        .def_readwrite("y", &sc_float3x3::y, "y")
        .def_readwrite("z", &sc_float3x3::z, "z");

    py::class_<sc_2d_pixel> pixel2D(m, "pixel2D"); // No docstring in C++
    pixel2D.def(py::init<>())
        .def(py::init<>())
        .def_readwrite("i", &sc_2d_pixel::i, "i")
        .def_readwrite("j", &sc_2d_pixel::j, "j");

    py::class_<rs2_safety_extrinsics_table> safety_extrinsics_table(m, "safety_extrinsics_table"); // No docstring in C++
    safety_extrinsics_table.def(py::init<>())
        .def(py::init<sc_float3x3, sc_float3>())
        .def_readwrite("rotation", &rs2_safety_extrinsics_table::rotation, "Rotation Value")
        .def_readwrite("translation", &rs2_safety_extrinsics_table::translation, "Translation Value");

    py::class_<rs2_safety_preset_header> safety_preset_header(m, "safety_preset_header"); // No docstring in C++
    safety_preset_header.def(py::init<>())
        .def_readwrite("version", &rs2_safety_preset_header::version, "Version")
        .def_readwrite("table_type", &rs2_safety_preset_header::table_type, "Table Type")
        .def_readwrite("table_size", &rs2_safety_preset_header::table_size, "Table Size")
        .def_readwrite("crc32", &rs2_safety_preset_header::crc32, "CRC32");

    py::class_<rs2_safety_platform> safety_platform(m, "safety_platform"); // No docstring in C++
    safety_platform.def(py::init<>())
        .def_readwrite("transformation_link", &rs2_safety_platform::transformation_link, "Transformation Link")
        .def_readwrite("robot_height", &rs2_safety_platform::robot_height, "Robot Height")
        .def_readwrite("robot_mass", &rs2_safety_platform::robot_mass, "Robot Mass")
        .def_property(BIND_RAW_ARRAY_PROPERTY(rs2_safety_platform, reserved, uint8_t, sizeof(rs2_safety_platform::reserved)), "Reserved");

    py::class_<rs2_safety_zone> safety_zone(m, "safety_zone"); // No docstring in C++
    safety_zone.def(py::init<>())
        .def_property("zone_polygon",
            [](const rs2_safety_zone& self)
            {
                return reinterpret_cast<const std::array<sc_float2, 4>&> (self.zone_polygon);
            },
            [](rs2_safety_zone& self, std::array< sc_float2, 4> arr) {
                for (int i = 0; i < 4; i++)
                {
                    self.zone_polygon[i] = arr[i];
                }
            },
            "Zone Polygon")
        .def_readwrite("safety_trigger_confidence", &rs2_safety_zone::safety_trigger_confidence, "Safety Trigger Confidence")
        .def_readwrite("minimum_object_size", &rs2_safety_zone::minimum_object_size, "Minimum Object Size")
        .def_readwrite("mos_target_type", &rs2_safety_zone::mos_target_type, "MOS Target Type")
        .def_property(BIND_RAW_ARRAY_PROPERTY(rs2_safety_zone, reserved, uint8_t, sizeof(rs2_safety_zone::reserved)), "Reserved");

    py::class_<rs2_safety_2d_masking_zone> masking_zone(m, "masking_zone"); // No docstring in C++
    masking_zone.def(py::init<>())
        .def_readwrite("attributes", &rs2_safety_2d_masking_zone::attributes, "Attributes")
        .def_readwrite("minimal_range", &rs2_safety_2d_masking_zone::minimal_range, "Minimal Range")
        .def_property("region_of_interests",
            [](const rs2_safety_2d_masking_zone& self)
            {
                return reinterpret_cast<const std::array<sc_2d_pixel, 4>&> (self.region_of_interests);
            },
            [](rs2_safety_2d_masking_zone& self, std::array< sc_2d_pixel, 4> arr) {
                for (int i = 0; i < 4; i++)
                {
                    self.region_of_interests[i] = arr[i];
                }
            },
            "Region Of Interests");

    py::class_<rs2_safety_environment> safety_environment(m, "safety_environment"); // No docstring in C++
    safety_environment.def(py::init<>())
        .def_readwrite( "grid_cell_size", &rs2_safety_environment::grid_cell_size, "Grid Cell Size" )
        .def_readwrite( "safety_trigger_duration", &rs2_safety_environment::safety_trigger_duration, "Safety Trigger Duration" )
        .def_readwrite( "linear_velocity", &rs2_safety_environment::linear_velocity, "Linear Velocity" )
        .def_readwrite( "angular_velocity", &rs2_safety_environment::angular_velocity, "Angular Velocity" )
        .def_readwrite( "payload_weight", &rs2_safety_environment::payload_weight, "Payload Weight" )
        .def_readwrite( "surface_inclination", &rs2_safety_environment::surface_inclination, "Surface Inclination" )
        .def_readwrite( "surface_height", &rs2_safety_environment::surface_height, "Surface Height" )
        .def_readwrite( "surface_confidence", &rs2_safety_environment::surface_confidence, "Surface Confidence" )
        .def_readwrite( "floor_fill_threshold", &rs2_safety_environment::floor_fill_threshold, "Floor fill threshold" )
        .def_readwrite( "depth_fill_threshold", &rs2_safety_environment::depth_fill_threshold, "Depth fill threshold" )
        .def_readwrite( "surface_height_threshold", &rs2_safety_environment::surface_height_threshold, "Surface height threshold" )
        .def_readwrite( "vision_hara_persistency", &rs2_safety_environment::vision_hara_persistency, "Vision HaRa persistency" )
        .def_property( BIND_RAW_ARRAY_PROPERTY( rs2_safety_environment, reserved, uint8_t, sizeof( rs2_safety_environment::reserved ) ), "Reserved" );

    py::class_<rs2_safety_preset> safety_preset(m, "safety_preset"); // No docstring in C++
    safety_preset.def(py::init<>())
        .def_readwrite("platform_config", &rs2_safety_preset::platform_config, "Platform Config")
        .def_property("safety_zones",
            [](const rs2_safety_preset& self)
            {
                return reinterpret_cast<const std::array<rs2_safety_zone, 2>&> (self.safety_zones);
            },
            [](rs2_safety_preset& self, std::array< rs2_safety_zone, 2> arr)
            {
                self.safety_zones[0] = arr[0];
                self.safety_zones[1] = arr[1];
            },
            "Safety Zones")
        .def_property("masking_zones",
            [](const rs2_safety_preset& self)
            {
                return reinterpret_cast<const std::array<rs2_safety_2d_masking_zone, 8>&> (self.masking_zones);
            },
            [](rs2_safety_preset& self, std::array< rs2_safety_2d_masking_zone, 8> arr)
            {
                for (int i = 0; i < 8; i++)
                {
                    self.masking_zones[i] = arr[i];
                }
            },
            "Masking Zones")
        .def_property(BIND_RAW_ARRAY_PROPERTY(rs2_safety_preset, reserved, uint8_t, sizeof(rs2_safety_preset::reserved)), "Reserved")
        .def_readwrite("environment", &rs2_safety_preset::environment, "Environment")
        .def("__eq__", [](const rs2_safety_preset& self, const rs2_safety_preset& other) {
                return self == other;
        })
        .def("__repr__", [](const rs2_safety_preset& self) {
            std::stringstream ss;
            ss << self;
            return ss.str();
        });

        BIND_ENUM(m, rs2_safety_mode, RS2_SAFETY_MODE_COUNT, "Safety Mode")
        BIND_ENUM(m, rs2_safety_pin_direction, RS2_SAFETY_PIN_DIRECTION_COUNT, "Safety Config Interface Pin Direction")
        BIND_ENUM(m, rs2_safety_pin_functionality, RS2_SAFETY_PIN_FUNCTIONALITY_COUNT, "Safety Config Interface Pin Functionality")

        py::class_<rs2_safety_interface_config_header> safety_interface_config_header(m, "safety_interface_config_header"); // No docstring in C++
        safety_interface_config_header.def(py::init<>())
            .def_readwrite("version", &rs2_safety_interface_config_header::version, "Version")
            .def_readwrite("table_type", &rs2_safety_interface_config_header::table_type, "Table Type")
            .def_readwrite("table_size", &rs2_safety_interface_config_header::table_size, "Table Size")
            .def_readwrite("calib_version", &rs2_safety_interface_config_header::calib_version, "Calib Version")
            .def_readwrite("crc32", &rs2_safety_interface_config_header::crc32, "CRC32");

        py::class_<rs2_safety_interface_config_pin> safety_interface_config_pin(m, "safety_interface_config_pin"); // No docstring in C++
        safety_interface_config_pin.def(py::init<>())
            .def(py::init<rs2_safety_pin_direction, rs2_safety_pin_functionality>(), "direction"_a, "functionality"_a)
            .def_readwrite("direction", &rs2_safety_interface_config_pin::direction, "Direction")
            .def_readwrite("functionality", &rs2_safety_interface_config_pin::functionality, "Functionality");

        py::class_<rs2_safety_interface_config> safety_interface_config(m, "safety_interface_config"); // No docstring in C++
        safety_interface_config.def(py::init<>())
            .def_readwrite("power", &rs2_safety_interface_config::power, "power")
            .def_readwrite("ossd1_b", &rs2_safety_interface_config::ossd1_b, "ossd1_b")
            .def_readwrite("ossd1_a", &rs2_safety_interface_config::ossd1_a, "ossd1_a")
            .def_readwrite("preset3_a", &rs2_safety_interface_config::preset3_a, "preset3_a")
            .def_readwrite("preset3_b", &rs2_safety_interface_config::preset3_b, "preset3_b")
            .def_readwrite("preset4_a", &rs2_safety_interface_config::preset4_a, "preset4_a")
            .def_readwrite("preset1_b", &rs2_safety_interface_config::preset1_b, "preset1_b")
            .def_readwrite("preset1_a", &rs2_safety_interface_config::preset1_a, "preset1_a")
            .def_readwrite("gpio_0", &rs2_safety_interface_config::gpio_0, "gpio_0")
            .def_readwrite("gpio_1", &rs2_safety_interface_config::gpio_1, "gpio_1")
            .def_readwrite("gpio_3", &rs2_safety_interface_config::gpio_3, "gpio_3")
            .def_readwrite("gpio_2", &rs2_safety_interface_config::gpio_2, "gpio_2")
            .def_readwrite("preset2_b", &rs2_safety_interface_config::preset2_b, "preset2_b")
            .def_readwrite("gpio_4", &rs2_safety_interface_config::gpio_4, "gpio_4")
            .def_readwrite("preset2_a", &rs2_safety_interface_config::preset2_a, "preset2_a")
            .def_readwrite("preset4_b", &rs2_safety_interface_config::preset4_b, "preset4_b")
            .def_readwrite("ground", &rs2_safety_interface_config::ground, "ground")
            .def_readwrite("gpio_stabilization_interval", &rs2_safety_interface_config::gpio_stabilization_interval, "gpio_stabilization_interval")
            .def_readwrite("safety_zone_selection_overlap_time_period", 
                &rs2_safety_interface_config::safety_zone_selection_overlap_time_period, 
                "safety_zone_selection_overlap_time_period");
    /** end rs_safety_types.h **/
}
