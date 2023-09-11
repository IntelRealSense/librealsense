/* License: Apache 2.0. See LICENSE file in root directory.
Copyright(c) 2017 Intel Corporation. All Rights Reserved. */

#include "pyrealsense2.h"
#include <librealsense2/rs.h>
#include <iomanip>
#include <src/types.h>
#include "../include/librealsense2/hpp/rs_safety_types.hpp"

std::string make_pythonic_str(std::string str)
{
    std::transform(begin(str), end(str), begin(str), ::tolower); //Make lower case
    std::replace(begin(str), end(str), ' ', '_'); //replace spaces with underscore
    if (str == "6dof") //Currently this is the only enum that starts with a digit
    {
        return "six_dof";
    }
    return str;
}


void init_c_files(py::module &m) {
       /** Binding of rs2_* enums */
    BIND_ENUM(m, rs2_notification_category, RS2_NOTIFICATION_CATEGORY_COUNT, "Category of the librealsense notification.")
    // rs2_exception_type
    BIND_ENUM(m, rs2_distortion, RS2_DISTORTION_COUNT, "Distortion model: defines how pixel coordinates should be mapped to sensor coordinates.")
    BIND_ENUM(m, rs2_log_severity, RS2_LOG_SEVERITY_COUNT, "Severity of the librealsense logger.")
    BIND_ENUM(m, rs2_extension, RS2_EXTENSION_COUNT, "Specifies advanced interfaces (capabilities) objects may implement.")
    BIND_ENUM(m, rs2_matchers, RS2_MATCHER_COUNT, "Specifies types of different matchers.")
    BIND_ENUM(m, rs2_camera_info, RS2_CAMERA_INFO_COUNT, "This information is mainly available for camera debug and troubleshooting and should not be used in applications.")
    BIND_ENUM(m, rs2_stream, RS2_STREAM_COUNT, "Streams are different types of data provided by RealSense devices.")
    BIND_ENUM(m, rs2_format, RS2_FORMAT_COUNT, "A stream's format identifies how binary data is encoded within a frame.")
    BIND_ENUM(m, rs2_timestamp_domain, RS2_TIMESTAMP_DOMAIN_COUNT, "Specifies the clock in relation to which the frame timestamp was measured.")
    BIND_ENUM(m, rs2_frame_metadata_value, RS2_FRAME_METADATA_COUNT, "Per-Frame-Metadata is the set of read-only properties that might be exposed for each individual frame.")
    BIND_ENUM(m, rs2_calib_target_type, RS2_CALIB_TARGET_COUNT, "Calibration target type.")

    BIND_ENUM(m, rs2_option, RS2_OPTION_COUNT+1, "Defines general configuration controls. These can generally be mapped to camera UVC controls, and can be set / queried at any time unless stated otherwise.")
    // Without __repr__ and __str__, we get the default 'enum_base' showing '???'
    py_rs2_option.attr( "__repr__" ) = py::cpp_function(
        []( const py::object & arg ) -> py::str
        {
            auto type = py::type::handle_of( arg );
            py::object type_name = type.attr( "__name__" );
            py::int_ arg_int( arg );
            py::str enum_name( rs2_option_to_string( rs2_option( (int) arg_int ) ) );
            return py::str( "<{} {} '{}'>" ).format( std::move( type_name ), arg_int, enum_name );
        },
        py::name( "__repr__" ),
        py::is_method( py_rs2_option ) );
    py_rs2_option.attr( "__str__" ) = py::cpp_function(
        []( const py::object & arg ) -> py::str {
            py::int_ arg_int( arg );
            return rs2_option_to_string( rs2_option( (int) arg_int ) );
        },
        py::name( "name" ),
        py::is_method( py_rs2_option ) );
    // Force binding of deprecated (renamed) options that we still want to expose for backwards compatibility
    py_rs2_option.value("ambient_light", RS2_OPTION_AMBIENT_LIGHT);
    py_rs2_option.value( "lld_temperature", RS2_OPTION_LLD_TEMPERATURE );

    m.def( "option_from_string", &rs2_option_from_string );

    BIND_ENUM(m, rs2_l500_visual_preset, RS2_L500_VISUAL_PRESET_COUNT, "For L500 devices: provides optimized settings (presets) for specific types of usage.")
    BIND_ENUM(m, rs2_rs400_visual_preset, RS2_RS400_VISUAL_PRESET_COUNT, "For D400 devices: provides optimized settings (presets) for specific types of usage.")
    BIND_ENUM(m, rs2_playback_status, RS2_PLAYBACK_STATUS_COUNT, "") // No docsDtring in C++
    BIND_ENUM(m, rs2_calibration_type, RS2_CALIBRATION_TYPE_COUNT, "Calibration type for use in device_calibration")
    BIND_ENUM_CUSTOM(m, rs2_calibration_status, RS2_CALIBRATION_STATUS_FIRST, RS2_CALIBRATION_STATUS_LAST, "Calibration callback status for use in device_calibration.trigger_device_calibration")

    /** rs_types.h **/
    py::class_<rs2_intrinsics> intrinsics(m, "intrinsics", "Video stream intrinsics.");
    intrinsics.def(py::init<>())
        .def_readwrite("width", &rs2_intrinsics::width, "Width of the image in pixels")
        .def_readwrite("height", &rs2_intrinsics::height, "Height of the image in pixels")
        .def_readwrite("ppx", &rs2_intrinsics::ppx, "Horizontal coordinate of the principal point of the image, as a pixel offset from the left edge")
        .def_readwrite("ppy", &rs2_intrinsics::ppy, "Vertical coordinate of the principal point of the image, as a pixel offset from the top edge")
        .def_readwrite("fx", &rs2_intrinsics::fx, "Focal length of the image plane, as a multiple of pixel width")
        .def_readwrite("fy", &rs2_intrinsics::fy, "Focal length of the image plane, as a multiple of pixel height")
        .def_readwrite("model", &rs2_intrinsics::model, "Distortion model of the image")
        .def_property(BIND_RAW_ARRAY_PROPERTY(rs2_intrinsics, coeffs, float, 5), "Distortion coefficients")
        .def("__repr__", [](const rs2_intrinsics& self) {
            std::ostringstream ss;
            ss << self;
            return ss.str();
        });

    py::class_<rs2_dsm_params> dsm_params( m, "dsm_params", "Video stream DSM parameters" );
    dsm_params.def( py::init<>() )
        .def_readonly( "timestamp", &rs2_dsm_params::timestamp, "seconds since epoch" )
        .def_readonly( "version", &rs2_dsm_params::version, "major<<12 | minor<<4 | patch" )
        .def_readwrite( "model", &rs2_dsm_params::model, "correction model (0/1/2 none/AOT/TOA)" )
        .def_property( BIND_RAW_ARRAY_PROPERTY( rs2_dsm_params, flags, uint8_t, sizeof( rs2_dsm_params::flags )), "flags" )
        .def_readwrite( "h_scale", &rs2_dsm_params::h_scale, "horizontal DSM scale" )
        .def_readwrite( "v_scale", &rs2_dsm_params::v_scale, "vertical DSM scale" )
        .def_readwrite( "h_offset", &rs2_dsm_params::h_offset, "horizontal DSM offset" )
        .def_readwrite( "v_offset", &rs2_dsm_params::v_offset, "vertical DSM offset" )
        .def_readwrite( "rtd_offset", &rs2_dsm_params::rtd_offset, "the Round-Trip-Distance delay" )
        .def_property_readonly( "temp", 
            []( rs2_dsm_params const & self ) -> float {
                           return float( self.temp_x2 ) / 2;
                       },
            "temperature (LDD for depth; HUM for color)" )
        .def_property( BIND_RAW_ARRAY_PROPERTY( rs2_dsm_params, reserved, uint8_t, sizeof( rs2_dsm_params::reserved )), "reserved" )
        .def( "__repr__",
            []( const rs2_dsm_params & self )
            {
                std::ostringstream ss;
                ss << self;
                return ss.str();
            } );

    py::class_<rs2_motion_device_intrinsic> motion_device_intrinsic(m, "motion_device_intrinsic", "Motion device intrinsics: scale, bias, and variances.");
    motion_device_intrinsic.def(py::init<>())
        .def_property(BIND_RAW_2D_ARRAY_PROPERTY(rs2_motion_device_intrinsic, data, float, 3, 4), "3x4 matrix with 3x3 scale and cross axis and 3x1 biases")
        .def_property(BIND_RAW_ARRAY_PROPERTY(rs2_motion_device_intrinsic, noise_variances, float, 3), "Variance of noise for X, Y, and Z axis")
        .def_property(BIND_RAW_ARRAY_PROPERTY(rs2_motion_device_intrinsic, bias_variances, float, 3), "Variance of bias for X, Y, and Z axis")
        .def("__repr__", [](const rs2_motion_device_intrinsic& self) {
            std::stringstream ss;
            ss << "data: " << matrix_to_string(self.data) << ", ";
            ss << "noise_variances: " << array_to_string(self.noise_variances) << ", ";
            ss << "bias_variances: " << array_to_string(self.bias_variances);
            return ss.str();
        });

    // rs2_vertex
    // rs2_pixel
    
    py::class_<rs2_vector> vector(m, "vector", "3D vector in Euclidean coordinate space.");
    vector.def(py::init<>())
        .def_readwrite("x", &rs2_vector::x)
        .def_readwrite("y", &rs2_vector::y)
        .def_readwrite("z", &rs2_vector::z)
        .def("__repr__", [](const rs2_vector& self) {
            std::stringstream ss;
            ss << "x: " << self.x << ", ";
            ss << "y: " << self.y << ", ";
            ss << "z: " << self.z;
            return ss.str();
        });

    py::class_<rs2_quaternion> quaternion(m, "quaternion", "Quaternion used to represent rotation.");
    quaternion.def(py::init<>())
        .def_readwrite("x", &rs2_quaternion::x)
        .def_readwrite("y", &rs2_quaternion::y)
        .def_readwrite("z", &rs2_quaternion::z)
        .def_readwrite("w", &rs2_quaternion::w)
        .def("__repr__", [](const rs2_quaternion& self) {
            std::stringstream ss;
            ss << "x: " << self.x << ", ";
            ss << "y: " << self.y << ", ";
            ss << "z: " << self.z << ", ";
            ss << "w: " << self.w;
            return ss.str();
        });

    py::class_<rs2_pose> pose(m, "pose"); // No docstring in C++
    pose.def(py::init<>())
        .def_readwrite("translation", &rs2_pose::translation, "X, Y, Z values of translation, in meters (relative to initial position)")
        .def_readwrite("velocity", &rs2_pose::velocity, "X, Y, Z values of velocity, in meters/sec")
        .def_readwrite("acceleration", &rs2_pose::acceleration, "X, Y, Z values of acceleration, in meters/sec^2")
        .def_readwrite("rotation", &rs2_pose::rotation, "Qi, Qj, Qk, Qr components of rotation as represented in quaternion rotation (relative to initial position)")
        .def_readwrite("angular_velocity", &rs2_pose::angular_velocity, "X, Y, Z values of angular velocity, in radians/sec")
        .def_readwrite("angular_acceleration", &rs2_pose::angular_acceleration, "X, Y, Z values of angular acceleration, in radians/sec^2")
        .def_readwrite("tracker_confidence", &rs2_pose::tracker_confidence, "Pose confidence 0x0 - Failed, 0x1 - Low, 0x2 - Medium, 0x3 - High")
        .def_readwrite("mapper_confidence", &rs2_pose::mapper_confidence, "Pose map confidence 0x0 - Failed, 0x1 - Low, 0x2 - Medium, 0x3 - High");
    /** end rs_types.h **/

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
        .def_readwrite("grid_cell_size", &rs2_safety_environment::grid_cell_size, "Grid Cell Size")
        .def_readwrite("safety_trigger_duration", &rs2_safety_environment::safety_trigger_duration, "Safety Trigger Duration")
        .def_readwrite("linear_velocity", &rs2_safety_environment::linear_velocity, "Linear Velocity")
        .def_readwrite("angular_velocity", &rs2_safety_environment::angular_velocity, "Angular Velocity")
        .def_readwrite("payload_weight", &rs2_safety_environment::payload_weight, "Payload Weight")
        .def_readwrite("surface_inclination", &rs2_safety_environment::surface_inclination, "Surface Inclination")
        .def_readwrite("surface_height", &rs2_safety_environment::surface_height, "Surface Height")
        .def_readwrite("surface_confidence", &rs2_safety_environment::surface_confidence, "Surface Confidence")
        .def_property(BIND_RAW_ARRAY_PROPERTY(rs2_safety_environment, reserved, uint8_t, sizeof(rs2_safety_environment::reserved)), "Reserved");

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


    /** end rs_safety_types.h **/

    /** rs_sensor.h **/
    py::class_<rs2_extrinsics> extrinsics(m, "extrinsics", "Cross-stream extrinsics: encodes the topology describing how the different devices are oriented.");
    extrinsics.def(py::init<>())
        .def_property(BIND_RAW_ARRAY_PROPERTY(rs2_extrinsics, rotation, float, 9), "Column - major 3x3 rotation matrix")
        .def_property(BIND_RAW_ARRAY_PROPERTY(rs2_extrinsics, translation, float, 3), "Three-element translation vector, in meters")
        .def("__repr__", [](const rs2_extrinsics &e) {
            std::stringstream ss;
            ss << "rotation: " << array_to_string(e.rotation);
            ss << "\ntranslation: " << array_to_string(e.translation);
            return ss.str();
        });
    /** end rs_sensor.h **/

}
