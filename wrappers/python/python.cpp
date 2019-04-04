/* License: Apache 2.0. See LICENSE file in root directory.
Copyright(c) 2017 Intel Corporation. All Rights Reserved. */

#include <pybind11/pybind11.h>

// convenience functions
#include <pybind11/operators.h>

// STL conversions
#include <pybind11/stl.h>

// std::chrono::*
#include <pybind11/chrono.h>

// makes certain STL containers opaque to prevent expensive copies
#include <pybind11/stl_bind.h>

// makes std::function conversions work
#include <pybind11/functional.h>

#include "../include/librealsense2/rs.h"
#include "../include/librealsense2/rs.hpp"
#include "../include/librealsense2/hpp/rs_export.hpp"
#include "../include/librealsense2/rs_advanced_mode.hpp"
#include "../include/librealsense2/rsutil.h"
#define NAME pyrealsense2
#define SNAME "pyrealsense2"
// hacky little bit of half-functions to make .def(BIND_DOWNCAST) look nice for binding as/is functions
#define BIND_DOWNCAST(class, downcast) "is_"#downcast, &rs2::class::is<rs2::downcast>).def("as_"#downcast, &rs2::class::as<rs2::downcast>
const std::string rs2_prefix{ "rs2_" };
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
#define BIND_ENUM(module, rs2_enum_type, RS2_ENUM_COUNT)                                                                    \
    static std::string rs2_enum_type##pyclass_name = std::string(#rs2_enum_type).substr(rs2_prefix.length());               \
    /* Above 'static' is required in order to keep the string alive since py::class_ does not copy it */                    \
    py::enum_<rs2_enum_type> py_##rs2_enum_type(module, rs2_enum_type##pyclass_name.c_str());                               \
    /* std::cout << std::endl << "## " << rs2_enum_type##pyclass_name  << ":" << std::endl; */                              \
    for (int i = 0; i < static_cast<int>(RS2_ENUM_COUNT); i++)                                                              \
    {                                                                                                                       \
        rs2_enum_type v = static_cast<rs2_enum_type>(i);                                                                    \
        const char* enum_name = rs2_enum_type##_to_string(v);                                                               \
        auto python_name = make_pythonic_str(enum_name);                                                                    \
        py_##rs2_enum_type.value(python_name.c_str(), v);                                                                   \
        /* std::cout << " - " << python_name << std::endl;    */                                                            \
    }

template<typename T, size_t SIZE>
void copy_raw_array(T(&dst)[SIZE], const std::array<T, SIZE>& src)
{
    for (size_t i = 0; i < SIZE; i++)
    {
        dst[i] = src[i];
    }
}

template<typename T, size_t NROWS, size_t NCOLS>
void copy_raw_2d_array(T(&dst)[NROWS][NCOLS], const std::array<std::array<T, NCOLS>, NROWS>& src)
{
    for (size_t i = 0; i < NROWS; i++)
    {
        for (size_t j = 0; j < NCOLS; j++)
        {
            dst[i][j] = src[i][j];
        }
    }
}
template <typename T, size_t N>
std::string array_to_string(const T(&arr)[N])
{
    std::ostringstream oss;
    oss << "[";
    for (int i = 0; i < N; i++)
    {
        if (i != 0)
            oss << ", ";
        oss << arr[i];
    }
    oss << "]";
    return oss.str();
}

#define BIND_RAW_ARRAY_GETTER(T, member, valueT, SIZE) [](const T& self) -> const std::array<valueT, SIZE>& { return reinterpret_cast<const std::array<valueT, SIZE>&>(self.member); }
#define BIND_RAW_ARRAY_SETTER(T, member, valueT, SIZE) [](T& self, const std::array<valueT, SIZE>& src) { copy_raw_array(self.member, src); }

#define BIND_RAW_2D_ARRAY_GETTER(T, member, valueT, NROWS, NCOLS) [](const T& self) -> const std::array<std::array<valueT, NCOLS>, NROWS>& { return reinterpret_cast<const std::array<std::array<valueT, NCOLS>, NROWS>&>(self.member); }
#define BIND_RAW_2D_ARRAY_SETTER(T, member, valueT, NROWS, NCOLS) [](T& self, const std::array<std::array<valueT, NCOLS>, NROWS>& src) { copy_raw_2d_array(self.member, src); }

#define BIND_RAW_ARRAY_PROPERTY(T, member, valueT, SIZE) #member, BIND_RAW_ARRAY_GETTER(T, member, valueT, SIZE), BIND_RAW_ARRAY_SETTER(T, member, valueT, SIZE)
#define BIND_RAW_2D_ARRAY_PROPERTY(T, member, valueT, NROWS, NCOLS) #member, BIND_RAW_2D_ARRAY_GETTER(T, member, valueT, NROWS, NCOLS), BIND_RAW_2D_ARRAY_SETTER(T, member, valueT, NROWS, NCOLS)

/*PYBIND11_MAKE_OPAQUE(std::vector<rs2::stream_profile>)*/

namespace py = pybind11;
using namespace pybind11::literals;

PYBIND11_MODULE(NAME, m) {
    m.doc() = R"pbdoc(
        LibrealsenseTM Python Bindings
        ==============================
        Library for accessing Intel RealSenseTM cameras
    )pbdoc";

    class BufData {
    public:
        void *_ptr = nullptr;         // Pointer to the underlying storage
        size_t _itemsize = 0;         // Size of individual items in bytes
        std::string _format;          // For homogeneous buffers, this should be set to format_descriptor<T>::format()
        size_t _ndim = 0;             // Number of dimensions
        std::vector<size_t> _shape;   // Shape of the tensor (1 entry per dimension)
        std::vector<size_t> _strides; // Number of entries between adjacent entries (for each per dimension)
    public:
        BufData(void *ptr, size_t itemsize, const std::string& format, size_t ndim, const std::vector<size_t> &shape, const std::vector<size_t> &strides)
            : _ptr(ptr), _itemsize(itemsize), _format(format), _ndim(ndim), _shape(shape), _strides(strides) {}
        BufData(void *ptr, size_t itemsize, const std::string& format, size_t size)
            : BufData(ptr, itemsize, format, 1, std::vector<size_t> { size }, std::vector<size_t> { itemsize }) { }
        BufData(void *ptr, // Raw data pointer
                size_t itemsize, // Size of the type in bytes
                const std::string& format, // Data type's format descriptor (e.g. "@f" for float xyz)
                size_t dim, // number of data elements per group (e.g. 3 for float xyz)
                size_t count) // Number of groups
            : BufData( ptr, itemsize, format, 2, std::vector<size_t> { count, dim }, std::vector<size_t> { itemsize*dim, itemsize })  { }
    };

    py::class_<BufData> BufData_py(m, "BufData", py::buffer_protocol());
    BufData_py.def_buffer([](BufData& self)
    { return py::buffer_info(
        self._ptr,
        self._itemsize,
        self._format,
        self._ndim,
        self._shape,
        self._strides); }
    );

    /**
    Binding of rs2_ enums
    */
    BIND_ENUM(m, rs2_camera_info, RS2_CAMERA_INFO_COUNT)
        BIND_ENUM(m, rs2_frame_metadata_value, RS2_FRAME_METADATA_COUNT)
        BIND_ENUM(m, rs2_stream, RS2_STREAM_COUNT)
        BIND_ENUM(m, rs2_extension, RS2_EXTENSION_COUNT)
        BIND_ENUM(m, rs2_format, RS2_FORMAT_COUNT)
        BIND_ENUM(m, rs2_notification_category, RS2_NOTIFICATION_CATEGORY_COUNT)
        BIND_ENUM(m, rs2_log_severity, RS2_LOG_SEVERITY_COUNT)
        BIND_ENUM(m, rs2_option, RS2_OPTION_COUNT)
        BIND_ENUM(m, rs2_timestamp_domain, RS2_TIMESTAMP_DOMAIN_COUNT)
        BIND_ENUM(m, rs2_distortion, RS2_DISTORTION_COUNT)
        BIND_ENUM(m, rs2_playback_status, RS2_PLAYBACK_STATUS_COUNT)

        py::class_<rs2_extrinsics> extrinsics(m, "extrinsics");
    extrinsics.def(py::init<>())
        .def_property(BIND_RAW_ARRAY_PROPERTY(rs2_extrinsics, rotation, float, 9))
        .def_property(BIND_RAW_ARRAY_PROPERTY(rs2_extrinsics, translation, float, 3))
        .def("__repr__", [](const rs2_extrinsics &e) {
        std::stringstream ss;
        ss << "rotation: " << array_to_string(e.rotation);
        ss << "\ntranslation: " << array_to_string(e.translation);
        return ss.str();
    });

    py::class_<rs2_intrinsics> intrinsics(m, "intrinsics");
    intrinsics.def(py::init<>())
        .def_readwrite("width", &rs2_intrinsics::width)
        .def_readwrite("height", &rs2_intrinsics::height)
        .def_readwrite("ppx", &rs2_intrinsics::ppx)
        .def_readwrite("ppy", &rs2_intrinsics::ppy)
        .def_readwrite("fx", &rs2_intrinsics::fx)
        .def_readwrite("fy", &rs2_intrinsics::fy)
        .def_readwrite("model", &rs2_intrinsics::model)
        .def_property(BIND_RAW_ARRAY_PROPERTY(rs2_intrinsics, coeffs, float, 5))
        .def("__repr__", [](const rs2_intrinsics& self)
    {
        std::stringstream ss;
        ss << "width: " << self.width << ", ";
        ss << "height: " << self.height << ", ";
        ss << "ppx: " << self.ppx << ", ";
        ss << "ppy: " << self.ppy << ", ";
        ss << "fx: " << self.fx << ", ";
        ss << "fy: " << self.fy << ", ";
        ss << "model: " << self.model << ", ";
        ss << "coeffs: " << array_to_string(self.coeffs);
        return ss.str();
    });

    py::class_<rs2_motion_device_intrinsic> motion_device_inrinsic(m, "motion_device_intrinsic");
    motion_device_inrinsic.def(py::init<>())
        .def_property(BIND_RAW_2D_ARRAY_PROPERTY(rs2_motion_device_intrinsic, data, float, 3, 4))
        .def_property(BIND_RAW_ARRAY_PROPERTY(rs2_motion_device_intrinsic, noise_variances, float, 3))
        .def_property(BIND_RAW_ARRAY_PROPERTY(rs2_motion_device_intrinsic, bias_variances, float, 3));

    /* rs2_types.hpp */
    py::class_<rs2::option_range> option_range(m, "option_range");
    option_range.def_readwrite("min", &rs2::option_range::min)
        .def_readwrite("max", &rs2::option_range::max)
        .def_readwrite("default", &rs2::option_range::def)
        .def_readwrite("step", &rs2::option_range::step)
        .def("__repr__", [](const rs2::option_range &self) {
        std::stringstream ss;
        ss << "<" SNAME ".option_range: " << self.min << "-" << self.max
            << "/" << self.step << " [" << self.def << "]>";
        return ss.str();
    });

    py::class_<rs2::region_of_interest> region_of_interest(m, "region_of_interest");
    region_of_interest.def_readwrite("min_x", &rs2::region_of_interest::min_x)
        .def_readwrite("min_y", &rs2::region_of_interest::min_y)
        .def_readwrite("max_x", &rs2::region_of_interest::max_x)
        .def_readwrite("max_y", &rs2::region_of_interest::max_y);

    /* rs2_context.hpp */
    py::class_<rs2::context> context(m, "context");
    context.def(py::init<>())
        .def("query_devices", (rs2::device_list(rs2::context::*)() const) &rs2::context::query_devices, "Create a static"
            " snapshot of all connected devices a the time of the call.")
        .def_property_readonly("devices", (rs2::device_list(rs2::context::*)() const) &rs2::context::query_devices,
            "Create a static snapshot of all connected devices a the time of the call.")
        .def("query_all_sensors", &rs2::context::query_all_sensors, "Generate a flat list of "
            "all available sensors from all RealSense devices.")
        .def_property_readonly("sensors", &rs2::context::query_all_sensors, "Generate a flat list of "
            "all available sensors from all RealSense devices.")
        .def("get_sensor_parent", &rs2::context::get_sensor_parent, "s"_a)
        .def("set_devices_changed_callback", [](rs2::context& self, std::function<void(rs2::event_information)> &callback)
    {
        self.set_devices_changed_callback(callback);
    }, "Register devices changed callback.", "callback"_a)
        // not binding create_processing_block, not inpr Python API.
        .def("load_device", &rs2::context::load_device, "Creates a devices from a RealSense file.\n"
            "On successful load, the device will be appended to the context and a devices_changed event triggered."
            "filename"_a)
        .def("unload_device", &rs2::context::unload_device, "filename"_a);

    /* rs2_device.hpp */
    py::class_<rs2::device> device(m, "device");
    device.def("query_sensors", &rs2::device::query_sensors, "Returns the list of adjacent devices, "
        "sharing the same physical parent composite device.")
        .def_property_readonly("sensors", &rs2::device::query_sensors, "Returns the list of adjacent devices, "
            "sharing the same physical parent composite device.")
        .def("first_depth_sensor", [](rs2::device& self) { return self.first<rs2::depth_sensor>(); })
        .def("first_roi_sensor", [](rs2::device& self) { return self.first<rs2::roi_sensor>(); })
        .def("first_pose_sensor", [](rs2::device& self) { return self.first<rs2::pose_sensor>(); })
        .def("supports", &rs2::device::supports, "Check if specific camera info is supported.", "info"_a)
        .def("get_info", &rs2::device::get_info, "Retrieve camera specific information, "
            "like versions of various internal components", "info"_a)
        .def("hardware_reset", &rs2::device::hardware_reset, "Send hardware reset request to the device")
        .def(py::init<>())
        .def("__nonzero__", &rs2::device::operator bool)
        .def(BIND_DOWNCAST(device, debug_protocol))
        .def(BIND_DOWNCAST(device, playback))
        .def(BIND_DOWNCAST(device, recorder))
        .def(BIND_DOWNCAST(device, tm2))
        .def("__repr__", [](const rs2::device &self)
    {
        std::stringstream ss;
        ss << "<" SNAME ".device: " << self.get_info(RS2_CAMERA_INFO_NAME)
            << " (S/N: " << self.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER)
            << ")>";
        return ss.str();
    });

    py::class_<rs2::debug_protocol> debug_protocol(m, "debug_protocol");
    debug_protocol.def(py::init<rs2::device>())
        .def("send_and_receive_raw_data", &rs2::debug_protocol::send_and_receive_raw_data,
            "input"_a);

    py::class_<rs2::device_list> device_list(m, "device_list");
    device_list.def(py::init<>())
        .def("contains", &rs2::device_list::contains)
        .def("__getitem__", [](const rs2::device_list& self, size_t i) {
        if (i >= self.size())
            throw py::index_error();
        return self[uint32_t(i)];
    })
        .def("__len__", &rs2::device_list::size)
        .def("size", &rs2::device_list::size)
        .def("__iter__", [](const rs2::device_list& self) {
        return py::make_iterator(self.begin(), self.end());
    }, py::keep_alive<0, 1>())
        .def("__getitem__", [](const rs2::device_list& self, py::slice slice) {
        size_t start, stop, step, slicelength;
        if (!slice.compute(self.size(), &start, &stop, &step, &slicelength))
            throw py::error_already_set();
        auto *dlist = new std::vector<rs2::device>(slicelength);
        for (size_t i = 0; i < slicelength; ++i) {
            (*dlist)[i] = self[uint32_t(start)];
            start += step;
        }
        return dlist;
    })
        .def("front", &rs2::device_list::front)
        .def("back", &rs2::device_list::back);

    // Not binding status_changed_callback, templated

    py::class_<rs2::event_information> event_information(m, "event_information");
    event_information.def("was_removed", &rs2::event_information::was_removed, "Check if "
        "specific device was disconnected.", "dev"_a)
        .def("was_added", &rs2::event_information::was_added, "Check if "
            "specific device was added.", "dev"_a)
        .def("get_new_devices", &rs2::event_information::get_new_devices, "Returns a "
            "list of all newly connected devices");

    py::class_<rs2::tm2, rs2::device> tm2(m, "tm2");
    tm2.def(py::init<rs2::device>(), "device"_a)
        .def("enable_loopback", &rs2::tm2::enable_loopback, "filename"_a)
        .def("disable_loopback", &rs2::tm2::disable_loopback)
        .def("is_loopback_enabled", &rs2::tm2::is_loopback_enabled)
        .def("connect_controller", &rs2::tm2::connect_controller, "mac_address"_a)
        .def("disconnect_controller", &rs2::tm2::disconnect_controller, "id"_a);


    /* rs2_frame.hpp */
    auto get_frame_data = [](const rs2::frame& self) ->  BufData
    {
        if (auto vf = self.as<rs2::video_frame>()) {
            std::map<size_t, std::string> bytes_per_pixel_to_format = { { 1, std::string("@B") },{ 2, std::string("@H") },{ 3, std::string("@I") },{ 4, std::string("@I") } };
            switch (vf.get_profile().format()) {
            case RS2_FORMAT_RGB8: case RS2_FORMAT_BGR8:
                return BufData(const_cast<void*>(vf.get_data()), 1, bytes_per_pixel_to_format[1], 3,
                    { static_cast<size_t>(vf.get_height()), static_cast<size_t>(vf.get_width()), 3 },
                    { static_cast<size_t>(vf.get_stride_in_bytes()), static_cast<size_t>(vf.get_bytes_per_pixel()), 1 });
                break;
            case RS2_FORMAT_RGBA8: case RS2_FORMAT_BGRA8:
                return BufData(const_cast<void*>(vf.get_data()), 1, bytes_per_pixel_to_format[1], 3,
                    { static_cast<size_t>(vf.get_height()), static_cast<size_t>(vf.get_width()), 4 },
                    { static_cast<size_t>(vf.get_stride_in_bytes()), static_cast<size_t>(vf.get_bytes_per_pixel()), 1 });
                break;
            default:
                return BufData(const_cast<void*>(vf.get_data()), static_cast<size_t>(vf.get_bytes_per_pixel()), bytes_per_pixel_to_format[vf.get_bytes_per_pixel()], 2,
                    { static_cast<size_t>(vf.get_height()), static_cast<size_t>(vf.get_width()) },
                    { static_cast<size_t>(vf.get_stride_in_bytes()), static_cast<size_t>(vf.get_bytes_per_pixel()) });
            }
        }
        else
            return BufData(const_cast<void*>(self.get_data()), 1, std::string("@B"), 0); };

    py::class_<rs2::frame> frame(m, "frame");
    frame.def(py::init<>())
        //         .def(py::self = py::self) // can't overload assignment in python
        .def(py::init<rs2::frame>())
        .def("swap", &rs2::frame::swap, "other"_a)
        .def("__nonzero__", &rs2::frame::operator bool)
        .def("get_timestamp", &rs2::frame::get_timestamp, "Retrieve the time at which the frame was captured")
        .def_property_readonly("timestamp", &rs2::frame::get_timestamp, "Retrieve the time at which the frame was captured")
        .def("get_frame_timestamp_domain", &rs2::frame::get_frame_timestamp_domain, "Retrieve the timestamp domain.")
        .def_property_readonly("frame_timestamp_domain", &rs2::frame::get_frame_timestamp_domain, "Retrieve the timestamp domain.")
        .def("get_frame_metadata", &rs2::frame::get_frame_metadata, "Retrieve the current value of a single frame_metadata.", "frame_metadata"_a)
        .def("supports_frame_metadata", &rs2::frame::supports_frame_metadata, "Determine if the device allows a specific metadata to be queried.", "frame_metadata"_a)
        .def("get_frame_number", &rs2::frame::get_frame_number, "Retrieve the frame number.")
        .def_property_readonly("frame_number", &rs2::frame::get_frame_number, "Retrieve the frame number.")
        .def("get_data", get_frame_data, "retrieve data from the frame handle.", py::keep_alive<0, 1>())
        .def_property_readonly("data", get_frame_data, "retrieve data from the frame handle.", py::keep_alive<0, 1>())
        .def("get_profile", &rs2::frame::get_profile)
        .def("keep", &rs2::frame::keep)
        .def_property_readonly("profile", &rs2::frame::get_profile)
        .def(BIND_DOWNCAST(frame, frame))
        .def(BIND_DOWNCAST(frame, points))
        .def(BIND_DOWNCAST(frame, frameset))
        .def(BIND_DOWNCAST(frame, video_frame))
        .def(BIND_DOWNCAST(frame, depth_frame))
        .def(BIND_DOWNCAST(frame, motion_frame))
        .def(BIND_DOWNCAST(frame, pose_frame));

    py::class_<rs2::video_frame, rs2::frame> video_frame(m, "video_frame");
    video_frame.def(py::init<rs2::frame>())
        .def("get_width", &rs2::video_frame::get_width, "Returns image width in pixels.")
        .def_property_readonly("width", &rs2::video_frame::get_width, "Returns image width in pixels.")
        .def("get_height", &rs2::video_frame::get_height, "Returns image height in pixels.")
        .def_property_readonly("height", &rs2::video_frame::get_height, "Returns image height in pixels.")
        .def("get_stride_in_bytes", &rs2::video_frame::get_stride_in_bytes, "Retrieve frame stride, meaning the actual line width in memory in bytes (not the logical image width).")
        .def_property_readonly("stride_in_bytes", &rs2::video_frame::get_stride_in_bytes, "Retrieve frame stride, meaning the actual line width in memory in bytes (not the logical image width).")
        .def("get_bits_per_pixel", &rs2::video_frame::get_bits_per_pixel, "Retrieve bits per pixel.")
        .def_property_readonly("bits_per_pixel", &rs2::video_frame::get_bits_per_pixel, "Retrieve bits per pixel.")
        .def("get_bytes_per_pixel", &rs2::video_frame::get_bytes_per_pixel, "Retrieve bytes per pixel.")
        .def("get_bytes_per_pixel", &rs2::video_frame::get_bytes_per_pixel, "Retrieve bytes per pixel.");


    py::class_<rs2_vector> vector(m, "vector");
    vector.def(py::init<>())
        .def_readwrite("x", &rs2_vector::x)
        .def_readwrite("y", &rs2_vector::y)
        .def_readwrite("z", &rs2_vector::z)
        .def("__repr__", [](const rs2_vector& self)
    {
        std::stringstream ss;
        ss << "x: " << self.x << ", ";
        ss << "y: " << self.y << ", ";
        ss << "z: " << self.z;
        return ss.str();
    });

    py::class_<rs2_quaternion> quaternion(m, "quaternion");
    quaternion.def(py::init<>())
        .def_readwrite("x", &rs2_quaternion::x)
        .def_readwrite("y", &rs2_quaternion::y)
        .def_readwrite("z", &rs2_quaternion::z)
        .def_readwrite("w", &rs2_quaternion::w)
        .def("__repr__", [](const rs2_quaternion& self)
    {
        std::stringstream ss;
        ss << "x: " << self.x << ", ";
        ss << "y: " << self.y << ", ";
        ss << "z: " << self.z << ", ";
        ss << "w: " << self.w;
        return ss.str();
    });


    py::class_<rs2_pose> pose(m, "pose");
    pose.def(py::init<>())
        .def_readwrite("translation",           &rs2_pose::translation)
        .def_readwrite("velocity",              &rs2_pose::velocity)
        .def_readwrite("acceleration",          &rs2_pose::acceleration)
        .def_readwrite("rotation",              &rs2_pose::rotation)
        .def_readwrite("angular_velocity",      &rs2_pose::angular_velocity)
        .def_readwrite("angular_acceleration",  &rs2_pose::angular_acceleration)
        .def_readwrite("tracker_confidence",    &rs2_pose::tracker_confidence)
        .def_readwrite("mapper_confidence",     &rs2_pose::mapper_confidence);

    py::class_<rs2::motion_frame, rs2::frame> motion_frame(m, "motion_frame");
    motion_frame.def(py::init<rs2::frame>())
        .def("get_motion_data", &rs2::motion_frame::get_motion_data, "Returns motion info of frame.");

    py::class_<rs2::pose_frame, rs2::frame> pose_frame(m, "pose_frame");
    pose_frame.def(py::init<rs2::frame>())
        .def("get_pose_data", &rs2::pose_frame::get_pose_data);

    py::class_<rs2::vertex> vertex(m, "vertex");
    vertex.def_readwrite("x", &rs2::vertex::x)
        .def_readwrite("y", &rs2::vertex::y)
        .def_readwrite("z", &rs2::vertex::z)
        .def(py::init([]() { return rs2::vertex{}; }))
        .def(py::init([](float x, float y, float z) { return rs2::vertex{ x,y,z }; }))
        .def("__repr__", [](const rs2::vertex& v) {
        std::ostringstream oss;
        oss << v.x << ", " << v.y << ", " << v.z;
        return oss.str();
    });

    py::class_<rs2::texture_coordinate> texture_coordinate(m, "texture_coordinate");
    texture_coordinate.def_readwrite("u", &rs2::texture_coordinate::u)
        .def_readwrite("v", &rs2::texture_coordinate::v)
        .def(py::init([]() { return rs2::texture_coordinate{}; }))
        .def(py::init([](float u, float v) { return rs2::texture_coordinate{ u, v }; }))
        .def("__repr__", [](const rs2::texture_coordinate& t) {
        std::ostringstream oss;
        oss << t.u << ", " << t.v;
        return oss.str();
    });

    py::class_<rs2::points, rs2::frame> points(m, "points");
    points.def(py::init<>())
        .def(py::init<rs2::frame>())
        .def("get_vertices", [](rs2::points& self, int dims) -> BufData
        {
            auto verts = const_cast<rs2::vertex*>(self.get_vertices());
            auto profile = self.get_profile().as<rs2::video_stream_profile>();
            size_t h = profile.height(), w = profile.width();
            switch (dims) {
            case 1:
                return BufData(verts, sizeof(rs2::vertex), "@fff", self.size());
            case 2:
                return BufData(verts, sizeof(float), "@f", 3, self.size());
            case 3:
                return BufData(verts, sizeof(float), "@f", 3, { h, w, 3 }, { w*3*sizeof(float), 3*sizeof(float), sizeof(float) });
            default:
                throw std::domain_error("dims arg only supports values of 1, 2 or 3");
            }
        }, py::keep_alive<0, 1>(), "dims"_a=1)
        .def("get_texture_coordinates", [](rs2::points& self, int dims) -> BufData
        {
            auto tex = const_cast<rs2::texture_coordinate*>(self.get_texture_coordinates());
            auto profile = self.get_profile().as<rs2::video_stream_profile>();
            size_t h = profile.height(), w = profile.width();
            switch (dims) {
            case 1:
                return BufData(tex, sizeof(rs2::texture_coordinate), "@ff", self.size());
            case 2:
                return BufData(tex, sizeof(float), "@f", 2, self.size());
            case 3:
                return BufData(tex, sizeof(float), "@f", 2, { h, w, 2 }, { w*2*sizeof(float), 2*sizeof(float), sizeof(float) });
            default:
                throw std::domain_error("dims arg only supports values of 1, 2 or 3");
            }
        }, py::keep_alive<0, 1>(), "dims"_a=1)
        .def("export_to_ply", &rs2::points::export_to_ply)
        .def("size", &rs2::points::size);

    py::class_<rs2::frameset, rs2::frame> frameset(m, "composite_frame");
    frameset.def(py::init<rs2::frame>())
        .def("first_or_default", &rs2::frameset::first_or_default, "s"_a, "f"_a = RS2_FORMAT_ANY)
        .def("first", &rs2::frameset::first, "s"_a, "f"_a = RS2_FORMAT_ANY)
        .def("size", &rs2::frameset::size)
        .def("foreach", [](const rs2::frameset& self, std::function<void(rs2::frame)> callable)
    {
        self.foreach(callable);
    })
        .def("__getitem__", &rs2::frameset::operator[])
        .def("get_depth_frame", &rs2::frameset::get_depth_frame)
        .def("get_color_frame", &rs2::frameset::get_color_frame)
        .def("get_infrared_frame", &rs2::frameset::get_infrared_frame, "index"_a = 0)
        .def("get_fisheye_frame", &rs2::frameset::get_fisheye_frame)
        //.def("get_pose_frame", &rs2::frameset::get_pose_frame)
        .def("get_pose_frame", [](rs2::frameset& self){   return self.get_pose_frame(); })
        .def("__iter__", [](rs2::frameset& self)
    {
        return py::make_iterator(self.begin(), self.end());
    }, py::keep_alive<0, 1>())
        .def("size", &rs2::frameset::size)
        .def("__getitem__", &rs2::frameset::operator[]);

    py::class_<rs2::depth_frame, rs2::video_frame> depth_frame(m, "depth_frame");
    depth_frame.def(py::init<rs2::frame>())
        .def("get_distance", &rs2::depth_frame::get_distance, "x"_a, "y"_a);

    /* rs2_processing.hpp */
    py::class_<rs2::filter_interface> filter_interface(m, "filter_interface");
    filter_interface.def("process", &rs2::filter_interface::process, "frame"_a);

    // Base class for options interface. Should be used via sensor
    py::class_<rs2::options> options(m, "options");
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
        .def("get_supported_options", &rs2::options::get_supported_options, "Retrieve list of supported options, "
            "of a supported option");

    /* rs2_processing.hpp */
    py::class_<rs2::frame_source> frame_source(m, "frame_source");
    frame_source.def("allocate_video_frame", &rs2::frame_source::allocate_video_frame,
                     "profile"_a, "original"_a, "new_bpp"_a = 0, "new_width"_a = 0,
                     "new_height"_a = 0, "new_stride"_a = 0, "frame_type"_a = RS2_EXTENSION_VIDEO_FRAME)
                .def("allocate_points", &rs2::frame_source::allocate_points, "profile"_a,
                     "original"_a)
                .def("allocate_composite_frame", &rs2::frame_source::allocate_composite_frame,
                     "frames"_a) // does anything special need to be done for the vector argument?
                .def("frame_ready", &rs2::frame_source::frame_ready, "result"_a);

    py::class_<rs2::frame_queue> frame_queue(m, "frame_queue");
    frame_queue.def(py::init<unsigned int>(), "Create a frame queue. Frame queues are the simplest "
                    "cross-platform synchronization primitive provided by librealsense to help "
                    "developers who are not using async APIs.")
               .def(py::init<>())
               .def("enqueue", &rs2::frame_queue::enqueue, "Enqueue a new frame into a queue.", "f"_a)
               .def("wait_for_frame", [](const rs2::frame_queue& self, unsigned int timeout_ms) { py::gil_scoped_release(); self.wait_for_frame(timeout_ms); }, "Wait until a new frame "
                    "becomes available in the queue and dequeue it.", "timeout_ms"_a = 5000)
               .def("poll_for_frame", [](const rs2::frame_queue &self)
                    {
                        rs2::frame frame;
                        self.poll_for_frame(&frame);
                        return frame;
                    }, "Poll if a new frame is available and dequeue it if it is")
               .def("try_wait_for_frame", [](const rs2::frame_queue &self, unsigned int timeout_ms)
                    {
                        rs2::frame frame;
                        auto success = self.try_wait_for_frame(&frame, timeout_ms);
                        return std::make_tuple(success, frame);
                    }, "timeout_ms"_a=5000)
               .def("__call__", &rs2::frame_queue::operator())
               .def("capacity", &rs2::frame_queue::capacity);

    // Not binding frame_processor_callback, templated
    py::class_<rs2::processing_block, rs2::options> processing_block(m, "processing_block");
    processing_block.def("__init__", [](rs2::processing_block &self, std::function<void(rs2::frame, rs2::frame_source&)> processing_function) {
        new (&self) rs2::processing_block(processing_function);
    }, "processing_function"_a);
    processing_block.def("start", [](rs2::processing_block& self, std::function<void(rs2::frame)> f)
                         {
                             self.start(f);
                         }, "callback"_a)
                    .def("invoke", &rs2::processing_block::invoke, "f"_a)
                  /*.def("__call__", &rs2::processing_block::operator(), "f"_a)*/;

    py::class_ <rs2::filter, rs2::processing_block, rs2::filter_interface> filter(m, "filter");
    filter.def("__init__", [](rs2::filter &self, std::function<void(rs2::frame, rs2::frame_source&)> filter_function, int queue_size){
        new (&self) rs2::filter(filter_function, queue_size);
    }, "filter_function"_a, "queue_size"_a = 1);

    // Not binding syncer_processing_block, not in Python API

    py::class_<rs2::pointcloud, rs2::filter> pointcloud(m, "pointcloud");
  
    pointcloud.def(py::init<>())
        .def(py::init<rs2_stream, int>(), "stream"_a, "index"_a = 0)
        .def("calculate", &rs2::pointcloud::calculate, "depth"_a)
        .def("map_to", &rs2::pointcloud::map_to, "mapped"_a);

    py::class_<rs2::syncer> syncer(m, "syncer");
    syncer.def(py::init<int>(), "queue_size"_a = 1)
        .def("wait_for_frames", &rs2::syncer::wait_for_frames, "Wait until a coherent set "
            "of frames becomes available", "timeout_ms"_a = 5000)
        .def("poll_for_frames", [](const rs2::syncer &self)
        {
            rs2::frameset frames;
            self.poll_for_frames(&frames);
            return frames;
        }, "Check if a coherent set of frames is available")
        .def("try_wait_for_frames", [](const rs2::syncer &self, unsigned int timeout_ms)
        {
            rs2::frameset fs;
            auto success = self.try_wait_for_frames(&fs, timeout_ms);
            return std::make_tuple(success, fs);
        }, "timeout_ms"_a = 5000);
        /*.def("__call__", &rs2::syncer::operator(), "frame"_a)*/

    py::class_<rs2::threshold_filter, rs2::filter> threshold(m, "threshold_filter");
    threshold.def(py::init<>())
		.def(py::init<float, float>(), "min_dist"_a, "max_dist"_a);


    py::class_<rs2::colorizer, rs2::filter> colorizer(m, "colorizer");
    colorizer.def(py::init<>())
        .def(py::init<float>(), "color_scheme"_a)
        .def("colorize", &rs2::colorizer::colorize, "depth"_a)
        /*.def("__call__", &rs2::colorizer::operator())*/;

    py::class_<rs2::align, rs2::filter> align(m, "align");
    align.def(py::init<rs2_stream>(), "align_to"_a)
        .def("process", (rs2::frameset (rs2::align::*)(rs2::frameset)) &rs2::align::process, "frames"_a);

    py::class_<rs2::decimation_filter, rs2::filter> decimation_filter(m, "decimation_filter");
    decimation_filter.def(py::init<>())
        .def(py::init<float>(), "magnitude"_a);

    py::class_<rs2::temporal_filter, rs2::filter> temporal_filter(m, "temporal_filter");
    temporal_filter.def(py::init<>())
        .def(py::init<float, float, int>(), "smooth_alpha"_a, "smooth_delta"_a, "persistence_control"_a);

    py::class_<rs2::spatial_filter, rs2::filter> spatial_filter(m, "spatial_filter");
    spatial_filter.def(py::init<>())
        .def(py::init<float, float, float, float>(), "smooth_alpha"_a, "smooth_delta"_a, "magnitude"_a, "hole_fill"_a);;

    py::class_<rs2::hole_filling_filter, rs2::filter> hole_filling_filter(m, "hole_filling_filter");
    hole_filling_filter.def(py::init<>())
        .def(py::init<int>(), "mode"_a);

    py::class_<rs2::disparity_transform, rs2::filter> disparity_transform(m, "disparity_transform");
    disparity_transform.def(py::init<bool>(), "transform_to_disparity"_a=true);

    py::class_<rs2::yuy_decoder, rs2::filter> yuy_decoder(m, "yuy_decoder");
    yuy_decoder.def(py::init<>());

    py::class_<rs2::zero_order_invalidation, rs2::filter> zero_order_invalidation(m, "zero_order_invalidation");
    zero_order_invalidation.def(py::init<>());

    /* rs_export.hpp */
    // py::class_<rs2::save_to_ply, rs2::filter> save_to_ply(m, "save_to_ply");
    // save_to_ply.def(py::init<std::string, rs2::pointcloud>(), "filename"_a = "RealSense Pointcloud ", "pc"_a = rs2::pointcloud())
    //            .def_readonly_static("option_ignore_color", &rs2::save_to_ply::OPTION_IGNORE_COLOR);

    py::class_<rs2::save_single_frameset, rs2::filter> save_single_frameset(m, "save_single_frameset");
    save_single_frameset.def(py::init<std::string>(), "filename"_a = "RealSense Frameset ");

    /* rs2_record_playback.hpp */
    py::class_<rs2::playback, rs2::device> playback(m, "playback");
    playback.def(py::init<rs2::device>(), "device"_a)
        .def("pause", &rs2::playback::pause)
        .def("resume", &rs2::playback::resume)
        .def("file_name", &rs2::playback::file_name)
        .def("get_position", &rs2::playback::get_position)
        .def("get_duration", &rs2::playback::get_duration)
        .def("seek", &rs2::playback::seek, "time"_a)
        .def("is_real_time", &rs2::playback::is_real_time)
        .def("set_real_time", &rs2::playback::set_real_time, "real_time"_a)
        .def("set_status_changed_callback", [](rs2::playback& self, std::function<void(rs2_playback_status)> callback)
    { self.set_status_changed_callback(callback); }, "callback"_a)
        .def("current_status", &rs2::playback::current_status);

    py::class_<rs2::recorder, rs2::device> recorder(m, "recorder");
    recorder.def(py::init<const std::string&, rs2::device>())
        .def("pause", &rs2::recorder::pause)
        .def("resume", &rs2::recorder::resume);

    /* rs2_sensor.hpp */
    py::class_<rs2::stream_profile> stream_profile(m, "stream_profile");
    stream_profile.def(py::init<>())
        .def("stream_index", &rs2::stream_profile::stream_index)
        .def("stream_type", &rs2::stream_profile::stream_type)
        .def("format", &rs2::stream_profile::format)
        .def("fps", &rs2::stream_profile::fps)
        .def("unique_id", &rs2::stream_profile::unique_id)
        .def("clone", &rs2::stream_profile::clone, "type"_a, "index"_a, "format"_a)
        .def(BIND_DOWNCAST(stream_profile, stream_profile))
        .def(BIND_DOWNCAST(stream_profile, video_stream_profile))
        .def(BIND_DOWNCAST(stream_profile, motion_stream_profile))
        .def("stream_name", &rs2::stream_profile::stream_name)
        .def("is_default", &rs2::stream_profile::is_default)
        .def("__nonzero__", &rs2::stream_profile::operator bool)
        .def("get_extrinsics_to", &rs2::stream_profile::get_extrinsics_to, "to"_a)
        .def("register_extrinsics_to", &rs2::stream_profile::register_extrinsics_to, "to"_a, "extrinsics"_a)
        .def("__repr__", [](const rs2::stream_profile& self)
    {
        std::stringstream ss;
        if (auto vf = self.as<rs2::video_stream_profile>())
        {
            ss << "<" SNAME ".video_stream_profile: "
                << vf.stream_type() << "(" << vf.stream_index() << ") " << vf.width()
                << "x" << vf.height() << " @ " << vf.fps() << "fps "
                << vf.format() << ">";
        }
        else
        {
            ss << "<" SNAME ".stream_profile: " << self.stream_type() << "(" << self.stream_index()
                << ") @ " << self.fps() << "fps " << self.format() << ">";
        }
        return ss.str();
    });

    py::class_<rs2::video_stream_profile, rs2::stream_profile> video_stream_profile(m, "video_stream_profile");
    video_stream_profile.def(py::init<const rs2::stream_profile&>(), "sp"_a)
        .def("width", &rs2::video_stream_profile::width)
        .def("height", &rs2::video_stream_profile::height)
        .def("get_intrinsics", &rs2::video_stream_profile::get_intrinsics)
        .def_property_readonly("intrinsics", &rs2::video_stream_profile::get_intrinsics)
        .def("__repr__", [](const rs2::video_stream_profile& self)
    {
        std::stringstream ss;
        ss << "<" SNAME ".video_stream_profile: "
            << self.stream_type() << "(" << self.stream_index() << ") " << self.width()
            << "x" << self.height() << " @ " << self.fps() << "fps "
            << self.format() << ">";
        return ss.str();
    });

    py::class_<rs2::motion_stream_profile, rs2::stream_profile> motion_stream_profile(m, "motion_stream_profile");
    motion_stream_profile.def(py::init<const rs2::stream_profile&>(), "sp"_a)
        .def("get_motion_intrinsics", &rs2::motion_stream_profile::get_motion_intrinsics, "Returns scale and bias of a motion stream.");

    py::class_<rs2::notification> notification(m, "notification");
    notification.def(py::init<>())
        .def("get_category", &rs2::notification::get_category,
            "Retrieve the notification's category.")
        .def("get_description", &rs2::notification::get_description,
            "Retrieve the notification's description.")
        .def("get_timestamp", &rs2::notification::get_timestamp,
            "Retrieve the notification's arrival timestamp.")
        .def("get_severity", &rs2::notification::get_severity,
            "Retrieve the notification's severity.")
        .def("get_serialized_data", &rs2::notification::get_severity,
            "Retrieve the notification's serialized data.")
        .def_property_readonly("category", &rs2::notification::get_category,
            "Retrieve the notification's category.")
        .def_property_readonly("description", &rs2::notification::get_description,
            "Retrieve the notification's description.")
        .def_property_readonly("timestamp", &rs2::notification::get_timestamp,
            "Retrieve the notification's arrival timestamp.")
        .def_property_readonly("severity", &rs2::notification::get_severity,
            "Retrieve the notification's severity.")
        .def_property_readonly("serialized_data", &rs2::notification::get_serialized_data,
            "Retrieve the notification's serialized data.")
        .def("__repr__", [](const rs2::notification &n) {
        return n.get_description();
    });

    // not binding notifications_callback, templated
    py::class_<rs2::sensor, rs2::options> sensor(m, "sensor");
    sensor.def("open", (void (rs2::sensor::*)(const rs2::stream_profile&) const) &rs2::sensor::open,
        "Open sensor for exclusive access, by commiting to a configuration", "profile"_a)
        .def("supports", (bool (rs2::sensor::*)(rs2_camera_info) const) &rs2::sensor::supports,
            "Check if specific camera info is supported.", "info")
        .def("supports", (bool (rs2::sensor::*)(rs2_option) const) &rs2::options::supports,
            "Check if specific camera info is supported.", "info")
        .def("get_info", &rs2::sensor::get_info, "Retrieve camera specific information, "
            "like versions of various internal components.", "info"_a)
        .def("set_notifications_callback", [](const rs2::sensor& self, std::function<void(rs2::notification)> callback)
    { self.set_notifications_callback(callback); }, "Register Notifications callback", "callback"_a)
        .def("open", (void (rs2::sensor::*)(const std::vector<rs2::stream_profile>&) const) &rs2::sensor::open,
            "Open sensor for exclusive access, by committing to a composite configuration, specifying one or "
            "more stream profiles.", "profiles"_a)
        .def("close", [](const rs2::sensor& self) { py::gil_scoped_release lock; self.close(); }, "Close sensor for exclusive access.")
        .def("start", [](const rs2::sensor& self, std::function<void(rs2::frame)> callback)
    { self.start(callback); }, "Start passing frames into user provided callback.", "callback"_a)
        .def("start", [](const rs2::sensor& self, rs2::frame_queue& queue) { self.start(queue); })
        .def("stop", [](const rs2::sensor& self) { py::gil_scoped_release lock; self.stop(); }, "Stop streaming.")
        .def("get_stream_profiles", &rs2::sensor::get_stream_profiles, "Check if physical sensor is supported.")
        .def("get_recommended_filters", &rs2::sensor::get_recommended_filters, "Return the recommended list of filters by the sensor.")

        .def_property_readonly("profiles", &rs2::sensor::get_stream_profiles, "Check if physical sensor is supported.")
        .def(py::init<>())
        .def("__nonzero__", &rs2::sensor::operator bool)
        .def(BIND_DOWNCAST(sensor, roi_sensor))
        .def(BIND_DOWNCAST(sensor, depth_sensor))
        .def(BIND_DOWNCAST(sensor, pose_sensor))
        .def(BIND_DOWNCAST(sensor, wheel_odometer));

    py::class_<rs2::roi_sensor, rs2::sensor> roi_sensor(m, "roi_sensor");
    roi_sensor.def(py::init<rs2::sensor>(), "sensor"_a)
        .def("set_region_of_interest", &rs2::roi_sensor::set_region_of_interest, "roi"_a)
        .def("get_region_of_interest", &rs2::roi_sensor::get_region_of_interest)
        .def("__nonzero__", &rs2::roi_sensor::operator bool);

    py::class_<rs2::depth_sensor, rs2::sensor> depth_sensor(m, "depth_sensor");
    depth_sensor.def(py::init<rs2::sensor>(), "sensor"_a)
        .def("get_depth_scale", &rs2::depth_sensor::get_depth_scale,
            "Retrieves mapping between the units of the depth image and meters.")
        .def("__nonzero__", &rs2::depth_sensor::operator bool);

    py::class_<rs2::pose_sensor, rs2::sensor> pose_sensor(m, "pose_sensor");
    pose_sensor.def(py::init<rs2::sensor>(), "sensor"_a)
        .def("import_localization_map", &rs2::pose_sensor::import_localization_map,
            "Load SLAM localization map from host to device.", "lmap_buf"_a)
        .def("export_localization_map", &rs2::pose_sensor::export_localization_map,
            "Extract SLAM localization map from device and store on host.")
        .def("set_static_node", &rs2::pose_sensor::set_static_node,
            "Create a named reference frame anchored to a specific 3D pose.")
        .def("get_static_node", &rs2::pose_sensor::get_static_node,
            "Retrieve a named reference frame anchored to a specific 3D pose.")
        .def("__nonzero__", &rs2::pose_sensor::operator bool);

    py::class_<rs2::wheel_odometer, rs2::sensor> wheel_odometer(m, "wheel_odometer");
    wheel_odometer.def(py::init<rs2::sensor>(), "sensor"_a)
        .def("load_wheel_odometery_config", &rs2::wheel_odometer::load_wheel_odometery_config,
            "odometry_config_buf"_a, "Load Wheel odometer settings from host to device.")
        .def("send_wheel_odometry", &rs2::wheel_odometer::send_wheel_odometry,
            "wo_sensor_id"_a, "frame_num"_a, "translational_velocity"_a,
            "Send wheel odometry data for each individual sensor (wheel)")
        .def("__nonzero__", &rs2::wheel_odometer::operator bool);

    /* rs2_pipeline.hpp */
    py::class_<rs2::pipeline> pipeline(m, "pipeline");
    pipeline.def(py::init<rs2::context>(), "ctx"_a = rs2::context())
        .def("start", (rs2::pipeline_profile(rs2::pipeline::*)(const rs2::config&)) &rs2::pipeline::start, "config"_a)
        .def("start", (rs2::pipeline_profile(rs2::pipeline::*)()) &rs2::pipeline::start)
        .def("start", [](rs2::pipeline& self, std::function<void(rs2::frame)> f) { self.start(f); }, "callback"_a)
        .def("stop", &rs2::pipeline::stop)
        .def("wait_for_frames", &rs2::pipeline::wait_for_frames, "timeout_ms"_a = 5000, py::call_guard<py::gil_scoped_release>())
        .def("poll_for_frames", [](const rs2::pipeline &self)
        {
            rs2::frameset frames;
            self.poll_for_frames(&frames);
            return frames;
        })
        .def("try_wait_for_frames", [](const rs2::pipeline &self, unsigned int timeout_ms)
        {
            rs2::frameset fs;
            auto success = self.try_wait_for_frames(&fs, timeout_ms);
            return std::make_tuple(success, fs);
        }, "timeout_ms"_a = 5000)
        .def("get_active_profile", &rs2::pipeline::get_active_profile);

    struct pipeline_wrapper //Workaround to allow python implicit conversion of pipeline to std::shared_ptr<rs2_pipeline>
    {
        std::shared_ptr<rs2_pipeline> _ptr;
    };

    py::class_<pipeline_wrapper>(m, "pipeline_wrapper")
        .def(py::init([](rs2::pipeline p) { return pipeline_wrapper{ p }; }));

    py::implicitly_convertible<rs2::pipeline, pipeline_wrapper>();

    py::class_<rs2::pipeline_profile> pipeline_profile(m, "pipeline_profile");
    pipeline_profile.def(py::init<>())
        .def("get_streams", &rs2::pipeline_profile::get_streams)
        .def("get_stream", &rs2::pipeline_profile::get_stream, "stream_type"_a, "stream_index"_a = -1)
        .def("get_device", &rs2::pipeline_profile::get_device);


    py::class_<rs2::config> config(m, "config");
    config.def(py::init<>())
        .def("enable_stream", (void (rs2::config::*)(rs2_stream, int, int, int, rs2_format, int)) &rs2::config::enable_stream, "stream_type"_a, "stream_index"_a, "width"_a, "height"_a, "format"_a = RS2_FORMAT_ANY, "framerate"_a = 0)
        .def("enable_stream", (void (rs2::config::*)(rs2_stream, int)) &rs2::config::enable_stream, "stream_type"_a, "stream_index"_a = -1)
        .def("enable_stream", (void (rs2::config::*)(rs2_stream, rs2_format, int))&rs2::config::enable_stream, "stream_type"_a, "format"_a, "framerate"_a = 0)
        .def("enable_stream", (void (rs2::config::*)(rs2_stream, int, int, rs2_format, int)) &rs2::config::enable_stream, "stream_type"_a, "width"_a, "height"_a, "format"_a = RS2_FORMAT_ANY, "framerate"_a = 0)
        .def("enable_stream", (void (rs2::config::*)(rs2_stream, int, rs2_format, int)) &rs2::config::enable_stream, "stream_type"_a, "stream_index"_a, "format"_a, "framerate"_a = 0)
        .def("enable_all_streams", &rs2::config::enable_all_streams)
        .def("enable_device", &rs2::config::enable_device, "serial"_a)
        .def("enable_device_from_file", &rs2::config::enable_device_from_file, "file_name"_a, "repeat_playback"_a = true)
        .def("enable_record_to_file", &rs2::config::enable_record_to_file, "file_name"_a)
        .def("disable_stream", &rs2::config::disable_stream, "stream"_a, "index"_a = -1)
        .def("disable_all_streams", &rs2::config::disable_all_streams)
        .def("resolve", [](rs2::config* c, pipeline_wrapper pw) -> rs2::pipeline_profile { return c->resolve(pw._ptr); })
        .def("can_resolve", [](rs2::config* c, pipeline_wrapper pw) -> bool { return c->can_resolve(pw._ptr); });

    /**
    RS400 Advanced Mode commands
    */

    py::class_<STDepthControlGroup> _STDepthControlGroup(m, "STDepthControlGroup");
    _STDepthControlGroup.def(py::init<>())
        .def_readwrite("plusIncrement", &STDepthControlGroup::plusIncrement)
        .def_readwrite("minusDecrement", &STDepthControlGroup::minusDecrement)
        .def_readwrite("deepSeaMedianThreshold", &STDepthControlGroup::deepSeaMedianThreshold)
        .def_readwrite("scoreThreshA", &STDepthControlGroup::scoreThreshA)
        .def_readwrite("scoreThreshB", &STDepthControlGroup::scoreThreshB)
        .def_readwrite("textureDifferenceThreshold", &STDepthControlGroup::textureDifferenceThreshold)
        .def_readwrite("textureCountThreshold", &STDepthControlGroup::textureCountThreshold)
        .def_readwrite("deepSeaSecondPeakThreshold", &STDepthControlGroup::deepSeaSecondPeakThreshold)
        .def_readwrite("deepSeaNeighborThreshold", &STDepthControlGroup::deepSeaNeighborThreshold)
        .def_readwrite("lrAgreeThreshold", &STDepthControlGroup::lrAgreeThreshold)
        .def("__repr__", [](const STDepthControlGroup &e) {
        std::stringstream ss;
        ss << "minusDecrement: " << e.minusDecrement << ", ";
        ss << "deepSeaMedianThreshold: " << e.deepSeaMedianThreshold << ", ";
        ss << "scoreThreshA: " << e.scoreThreshA << ", ";
        ss << "scoreThreshB: " << e.scoreThreshB << ", ";
        ss << "textureDifferenceThreshold: " << e.textureDifferenceThreshold << ", ";
        ss << "textureCountThreshold: " << e.textureCountThreshold << ", ";
        ss << "deepSeaSecondPeakThreshold: " << e.deepSeaSecondPeakThreshold << ", ";
        ss << "deepSeaNeighborThreshold: " << e.deepSeaNeighborThreshold << ", ";
        ss << "lrAgreeThreshold: " << e.lrAgreeThreshold;
        return ss.str();
    });

    py::class_<STRsm> _STRsm(m, "STRsm");
    _STRsm.def(py::init<>())
        .def_readwrite("rsmBypass", &STRsm::rsmBypass)
        .def_readwrite("diffThresh", &STRsm::diffThresh)
        .def_readwrite("sloRauDiffThresh", &STRsm::sloRauDiffThresh)
        .def_readwrite("removeThresh", &STRsm::removeThresh)
        .def("__repr__", [](const STRsm &e) {
        std::stringstream ss;
        ss << "rsmBypass: " << e.rsmBypass << ", ";
        ss << "diffThresh: " << e.diffThresh << ", ";
        ss << "sloRauDiffThresh: " << e.sloRauDiffThresh << ", ";
        ss << "removeThresh: " << e.removeThresh;
        return ss.str();
    });

    py::class_<STRauSupportVectorControl> _STRauSupportVectorControl(m, "STRauSupportVectorControl");
    _STRauSupportVectorControl.def(py::init<>())
        .def_readwrite("minWest", &STRauSupportVectorControl::minWest)
        .def_readwrite("minEast", &STRauSupportVectorControl::minEast)
        .def_readwrite("minWEsum", &STRauSupportVectorControl::minWEsum)
        .def_readwrite("minNorth", &STRauSupportVectorControl::minNorth)
        .def_readwrite("minSouth", &STRauSupportVectorControl::minSouth)
        .def_readwrite("minNSsum", &STRauSupportVectorControl::minNSsum)
        .def_readwrite("uShrink", &STRauSupportVectorControl::uShrink)
        .def_readwrite("vShrink", &STRauSupportVectorControl::vShrink)
        .def("__repr__", [](const STRauSupportVectorControl &e) {
        std::stringstream ss;
        ss << "minWest: " << e.minWest << ", ";
        ss << "minEast: " << e.minEast << ", ";
        ss << "minWEsum: " << e.minWEsum << ", ";
        ss << "minNorth: " << e.minNorth << ", ";
        ss << "minSouth: " << e.minSouth << ", ";
        ss << "minNSsum: " << e.minNSsum << ", ";
        ss << "uShrink: " << e.uShrink << ", ";
        ss << "vShrink: " << e.vShrink;
        return ss.str();
    });
    py::class_<STColorControl> _STColorControl(m, "STColorControl");
    _STColorControl.def(py::init<>())
        .def_readwrite("disableSADColor", &STColorControl::disableSADColor)
        .def_readwrite("disableRAUColor", &STColorControl::disableRAUColor)
        .def_readwrite("disableSLORightColor", &STColorControl::disableSLORightColor)
        .def_readwrite("disableSLOLeftColor", &STColorControl::disableSLOLeftColor)
        .def_readwrite("disableSADNormalize", &STColorControl::disableSADNormalize)
        .def("__repr__", [](const STColorControl &e) {
        std::stringstream ss;
        ss << "disableSADColor: " << e.disableSADColor << ", ";
        ss << "disableRAUColor: " << e.disableRAUColor << ", ";
        ss << "disableSLORightColor: " << e.disableSLORightColor << ", ";
        ss << "disableSLOLeftColor: " << e.disableSLOLeftColor << ", ";
        ss << "disableSADNormalize: " << e.disableSADNormalize;
        return ss.str();
    });

    py::class_<STRauColorThresholdsControl> _STRauColorThresholdsControl(m, "STRauColorThresholdsControl");
    _STRauColorThresholdsControl.def(py::init<>())
        .def_readwrite("rauDiffThresholdRed", &STRauColorThresholdsControl::rauDiffThresholdRed)
        .def_readwrite("rauDiffThresholdGreen", &STRauColorThresholdsControl::rauDiffThresholdGreen)
        .def_readwrite("rauDiffThresholdBlue", &STRauColorThresholdsControl::rauDiffThresholdBlue)
        .def("__repr__", [](const STRauColorThresholdsControl &e) {
        std::stringstream ss;
        ss << "rauDiffThresholdRed: " << e.rauDiffThresholdRed << ", ";
        ss << "rauDiffThresholdGreen: " << e.rauDiffThresholdGreen << ", ";
        ss << "rauDiffThresholdBlue: " << e.rauDiffThresholdBlue;
        return ss.str();
    });

    py::class_<STSloColorThresholdsControl> _STSloColorThresholdsControl(m, "STSloColorThresholdsControl");
    _STSloColorThresholdsControl.def(py::init<>())
        .def_readwrite("diffThresholdRed", &STSloColorThresholdsControl::diffThresholdRed)
        .def_readwrite("diffThresholdGreen", &STSloColorThresholdsControl::diffThresholdGreen)
        .def_readwrite("diffThresholdBlue", &STSloColorThresholdsControl::diffThresholdBlue)
        .def("__repr__", [](const STSloColorThresholdsControl &e) {
        std::stringstream ss;
        ss << "diffThresholdRed: " << e.diffThresholdRed << ", ";
        ss << "diffThresholdGreen: " << e.diffThresholdGreen << ", ";
        ss << "diffThresholdBlue: " << e.diffThresholdBlue;
        return ss.str();
    });
    py::class_<STSloPenaltyControl> _STSloPenaltyControl(m, "STSloPenaltyControl");
    _STSloPenaltyControl.def(py::init<>())
        .def_readwrite("sloK1Penalty", &STSloPenaltyControl::sloK1Penalty)
        .def_readwrite("sloK2Penalty", &STSloPenaltyControl::sloK2Penalty)
        .def_readwrite("sloK1PenaltyMod1", &STSloPenaltyControl::sloK1PenaltyMod1)
        .def_readwrite("sloK2PenaltyMod1", &STSloPenaltyControl::sloK2PenaltyMod1)
        .def_readwrite("sloK1PenaltyMod2", &STSloPenaltyControl::sloK1PenaltyMod2)
        .def_readwrite("sloK2PenaltyMod2", &STSloPenaltyControl::sloK2PenaltyMod2)
        .def("__repr__", [](const STSloPenaltyControl &e) {
        std::stringstream ss;
        ss << "sloK1Penalty: " << e.sloK1Penalty << ", ";
        ss << "sloK2Penalty: " << e.sloK2Penalty << ", ";
        ss << "sloK1PenaltyMod1: " << e.sloK1PenaltyMod1 << ", ";
        ss << "sloK2PenaltyMod1: " << e.sloK2PenaltyMod1 << ", ";
        ss << "sloK1PenaltyMod2: " << e.sloK1PenaltyMod2 << ", ";
        ss << "sloK2PenaltyMod2: " << e.sloK2PenaltyMod2;
        return ss.str();
    });
    py::class_<STHdad> _STHdad(m, "STHdad");
    _STHdad.def(py::init<>())
        .def_readwrite("lambdaCensus", &STHdad::lambdaCensus)
        .def_readwrite("lambdaAD", &STHdad::lambdaAD)
        .def_readwrite("ignoreSAD", &STHdad::ignoreSAD)
        .def("__repr__", [](const STHdad &e) {
        std::stringstream ss;
        ss << "lambdaCensus: " << e.lambdaCensus << ", ";
        ss << "lambdaAD: " << e.lambdaAD << ", ";
        ss << "ignoreSAD: " << e.ignoreSAD;
        return ss.str();
    });

    py::class_<STColorCorrection> _STColorCorrection(m, "STColorCorrection");
    _STColorCorrection.def(py::init<>())
        .def_readwrite("colorCorrection1", &STColorCorrection::colorCorrection1)
        .def_readwrite("colorCorrection2", &STColorCorrection::colorCorrection2)
        .def_readwrite("colorCorrection3", &STColorCorrection::colorCorrection3)
        .def_readwrite("colorCorrection4", &STColorCorrection::colorCorrection4)
        .def_readwrite("colorCorrection5", &STColorCorrection::colorCorrection5)
        .def_readwrite("colorCorrection6", &STColorCorrection::colorCorrection6)
        .def_readwrite("colorCorrection7", &STColorCorrection::colorCorrection7)
        .def_readwrite("colorCorrection8", &STColorCorrection::colorCorrection8)
        .def_readwrite("colorCorrection9", &STColorCorrection::colorCorrection9)
        .def_readwrite("colorCorrection10", &STColorCorrection::colorCorrection10)
        .def_readwrite("colorCorrection11", &STColorCorrection::colorCorrection11)
        .def_readwrite("colorCorrection12", &STColorCorrection::colorCorrection12)
        .def("__repr__", [](const STColorCorrection &e) {
        std::stringstream ss;
        ss << "colorCorrection1: " << e.colorCorrection1 << ", ";
        ss << "colorCorrection2: " << e.colorCorrection2 << ", ";
        ss << "colorCorrection3: " << e.colorCorrection3 << ", ";
        ss << "colorCorrection4: " << e.colorCorrection4 << ", ";
        ss << "colorCorrection5: " << e.colorCorrection5 << ", ";
        ss << "colorCorrection6: " << e.colorCorrection6 << ", ";
        ss << "colorCorrection7: " << e.colorCorrection7 << ", ";
        ss << "colorCorrection8: " << e.colorCorrection8 << ", ";
        ss << "colorCorrection9: " << e.colorCorrection9 << ", ";
        ss << "colorCorrection10: " << e.colorCorrection10 << ", ";
        ss << "colorCorrection11: " << e.colorCorrection11 << ", ";
        ss << "colorCorrection12: " << e.colorCorrection12;
        return ss.str();
    });

    py::class_<STAEControl> _STAEControl(m, "STAEControl");
    _STAEControl.def(py::init<>())
        .def_readwrite("meanIntensitySetPoint", &STAEControl::meanIntensitySetPoint)
        .def("__repr__", [](const STAEControl &e) {
        std::stringstream ss;
        ss << "Mean Intensity Set Point: " << e.meanIntensitySetPoint;
        return ss.str();
    });

    py::class_<STDepthTableControl> _STDepthTableControl(m, "STDepthTableControl");
    _STDepthTableControl.def(py::init<>())
        .def_readwrite("depthUnits", &STDepthTableControl::depthUnits)
        .def_readwrite("depthClampMin", &STDepthTableControl::depthClampMin)
        .def_readwrite("depthClampMax", &STDepthTableControl::depthClampMax)
        .def_readwrite("disparityMode", &STDepthTableControl::disparityMode)
        .def_readwrite("disparityShift", &STDepthTableControl::disparityShift)
        .def("__repr__", [](const STDepthTableControl &e) {
        std::stringstream ss;
        ss << "depthUnits: " << e.depthUnits << ", ";
        ss << "depthClampMin: " << e.depthClampMin << ", ";
        ss << "depthClampMax: " << e.depthClampMax << ", ";
        ss << "disparityMode: " << e.disparityMode << ", ";
        ss << "disparityShift: " << e.disparityShift;
        return ss.str();
    });

    py::class_<STCensusRadius> _STCensusRadius(m, "STCensusRadius");
    _STCensusRadius.def(py::init<>())
        .def_readwrite("uDiameter", &STCensusRadius::uDiameter)
        .def_readwrite("vDiameter", &STCensusRadius::vDiameter)
        .def("__repr__", [](const STCensusRadius &e) {
        std::stringstream ss;
        ss << "uDiameter: " << e.uDiameter << ", ";
        ss << "vDiameter: " << e.vDiameter;
        return ss.str();
    });

    py::class_<rs400::advanced_mode> rs400_advanced_mode(m, "rs400_advanced_mode");
    rs400_advanced_mode.def(py::init<rs2::device>(), "device"_a)
        .def("toggle_advanced_mode", &rs400::advanced_mode::toggle_advanced_mode, "enable"_a)
        .def("is_enabled", &rs400::advanced_mode::is_enabled)
        .def("set_depth_control", &rs400::advanced_mode::set_depth_control, "group"_a) //STDepthControlGroup
        .def("get_depth_control", &rs400::advanced_mode::get_depth_control, "mode"_a = 0)
        .def("set_rsm", &rs400::advanced_mode::set_rsm, "group") //STRsm
        .def("get_rsm", &rs400::advanced_mode::get_rsm, "mode"_a = 0)
        .def("set_rau_support_vector_control", &rs400::advanced_mode::set_rau_support_vector_control, "group"_a)//STRauSupportVectorControl
        .def("get_rau_support_vector_control", &rs400::advanced_mode::get_rau_support_vector_control, "mode"_a = 0)
        .def("set_color_control", &rs400::advanced_mode::set_color_control, "group"_a) //STColorControl
        .def("get_color_control", &rs400::advanced_mode::get_color_control, "mode"_a = 0)//STColorControl
        .def("set_rau_thresholds_control", &rs400::advanced_mode::set_rau_thresholds_control, "group"_a)//STRauColorThresholdsControl
        .def("get_rau_thresholds_control", &rs400::advanced_mode::get_rau_thresholds_control, "mode"_a = 0)
        .def("set_slo_color_thresholds_control", &rs400::advanced_mode::set_slo_color_thresholds_control, "group"_a)//STSloColorThresholdsControl
        .def("get_slo_color_thresholds_control", &rs400::advanced_mode::get_slo_color_thresholds_control, "mode"_a = 0)//STSloColorThresholdsControl
        .def("set_slo_penalty_control", &rs400::advanced_mode::set_slo_penalty_control, "group"_a) //STSloPenaltyControl
        .def("get_slo_penalty_control", &rs400::advanced_mode::get_slo_penalty_control, "mode"_a = 0)//STSloPenaltyControl
        .def("set_hdad", &rs400::advanced_mode::set_hdad, "group"_a) //STHdad
        .def("get_hdad", &rs400::advanced_mode::get_hdad, "mode"_a = 0)
        .def("set_color_correction", &rs400::advanced_mode::set_color_correction, "group"_a)
        .def("get_color_correction", &rs400::advanced_mode::get_color_correction, "mode"_a = 0) //STColorCorrection
        .def("set_depth_table", &rs400::advanced_mode::set_depth_table, "group"_a)
        .def("get_depth_table", &rs400::advanced_mode::get_depth_table, "mode"_a = 0) //STDepthTableControl
        .def("set_ae_control", &rs400::advanced_mode::set_ae_control, "group"_a)
        .def("get_ae_control", &rs400::advanced_mode::get_ae_control, "mode"_a = 0) //STAEControl
        .def("set_census", &rs400::advanced_mode::set_census, "group"_a)    //STCensusRadius
        .def("get_census", &rs400::advanced_mode::get_census, "mode"_a = 0) //STCensusRadius
        .def("serialize_json", &rs400::advanced_mode::serialize_json)
        .def("load_json", &rs400::advanced_mode::load_json, "json_content"_a);

    /* rs2.hpp */
    m.def("log_to_console", &rs2::log_to_console, "min_severity"_a);
    m.def("log_to_file", &rs2::log_to_file, "min_severity"_a, "file_path"_a);

    /* rsutil.h */
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
}
