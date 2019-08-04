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
#define BIND_ENUM(module, rs2_enum_type, RS2_ENUM_COUNT, docstring)                                                         \
    static std::string rs2_enum_type##pyclass_name = std::string(#rs2_enum_type).substr(rs2_prefix.length());               \
    /* Above 'static' is required in order to keep the string alive since py::class_ does not copy it */                    \
    py::enum_<rs2_enum_type> py_##rs2_enum_type(module, rs2_enum_type##pyclass_name.c_str(), docstring);                    \
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
    BIND_ENUM(m, rs2_camera_info, RS2_CAMERA_INFO_COUNT, "This information is mainly available for camera debug and troubleshooting and should not be used in applications.")
    BIND_ENUM(m, rs2_frame_metadata_value, RS2_FRAME_METADATA_COUNT, "Per-Frame-Metadata is the set of read-only properties that might be exposed for each individual frame.")
    BIND_ENUM(m, rs2_stream, RS2_STREAM_COUNT, "Streams are different types of data provided by RealSense devices.")
    BIND_ENUM(m, rs2_extension, RS2_EXTENSION_COUNT, "Specifies advanced interfaces (capabilities) objects may implement.")
    BIND_ENUM(m, rs2_format, RS2_FORMAT_COUNT, "A stream's format identifies how binary data is encoded within a frame.")
    BIND_ENUM(m, rs2_notification_category, RS2_NOTIFICATION_CATEGORY_COUNT, "Category of the librealsense notification.")
    BIND_ENUM(m, rs2_log_severity, RS2_LOG_SEVERITY_COUNT, "Severity of the librealsense logger.")
    BIND_ENUM(m, rs2_option, RS2_OPTION_COUNT, "Defines general configuration controls. These can generally be mapped to camera UVC controls, and can be set / queried at any time unless stated otherwise.")
    BIND_ENUM(m, rs2_timestamp_domain, RS2_TIMESTAMP_DOMAIN_COUNT, "Specifies the clock in relation to which the frame timestamp was measured.")
    BIND_ENUM(m, rs2_distortion, RS2_DISTORTION_COUNT, "Distortion model: defines how pixel coordinates should be mapped to sensor coordinates.")
    BIND_ENUM(m, rs2_playback_status, RS2_PLAYBACK_STATUS_COUNT, "") // No docstring in C++

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

    py::class_<rs2_motion_device_intrinsic> motion_device_inrinsic(m, "motion_device_intrinsic", "Motion device intrinsics: scale, bias, and variances.");
    motion_device_inrinsic.def(py::init<>())
        .def_property(BIND_RAW_2D_ARRAY_PROPERTY(rs2_motion_device_intrinsic, data, float, 3, 4), "Interpret data array values")
        .def_property(BIND_RAW_ARRAY_PROPERTY(rs2_motion_device_intrinsic, noise_variances, float, 3), "Variance of noise for X, Y, and Z axis")
        .def_property(BIND_RAW_ARRAY_PROPERTY(rs2_motion_device_intrinsic, bias_variances, float, 3), "Variance of bias for X, Y, and Z axis");

    /* rs2_types.hpp */
    py::class_<rs2::option_range> option_range(m, "option_range"); // No docstring in C++
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

    py::class_<rs2::region_of_interest> region_of_interest(m, "region_of_interest"); // No docstring in C++
    region_of_interest.def_readwrite("min_x", &rs2::region_of_interest::min_x)
        .def_readwrite("min_y", &rs2::region_of_interest::min_y)
        .def_readwrite("max_x", &rs2::region_of_interest::max_x)
        .def_readwrite("max_y", &rs2::region_of_interest::max_y);

    /* rs2_context.hpp */
    py::class_<rs2::context> context(m, "context", "Librealsense context class. Includes realsense API version.");
    context.def(py::init<>())
        .def("query_devices", (rs2::device_list(rs2::context::*)() const) &rs2::context::query_devices, "Create a static"
             " snapshot of all connected devices at the time of the call.")
        .def_property_readonly("devices", (rs2::device_list(rs2::context::*)() const) &rs2::context::query_devices,
                               "A static snapshot of all connected devices at time of access. Identical to calling query_devices.")
        .def("query_all_sensors", &rs2::context::query_all_sensors, "Generate a flat list of "
             "all available sensors from all RealSense devices.")
        .def_property_readonly("sensors", &rs2::context::query_all_sensors, "A flat list of "
                               "all available sensors from all RealSense devices. Identical to calling query_all_sensors.")
        .def("get_sensor_parent", &rs2::context::get_sensor_parent, "s"_a) // no docstring in C++
        .def("set_devices_changed_callback", [](rs2::context& self, std::function<void(rs2::event_information)> &callback) {
            self.set_devices_changed_callback(callback);
        }, "Register devices changed callback.", "callback"_a)
        // not binding create_processing_block, not inpr Python API.
        .def("load_device", &rs2::context::load_device, "Creates a devices from a RealSense file.\n"
             "On successful load, the device will be appended to the context and a devices_changed event triggered."
             "filename"_a)
        .def("unload_device", &rs2::context::unload_device, "filename"_a) // No docstring in C++
        .def("unload_tracking_module", &rs2::context::unload_tracking_module); // No docstring in C++

    /* rs2_device.hpp */
    py::class_<rs2::device> device(m, "device"); // No docstring in C++
    device.def("query_sensors", &rs2::device::query_sensors, "Returns the list of adjacent devices, "
               "sharing the same physical parent composite device.")
        .def_property_readonly("sensors", &rs2::device::query_sensors, "List of adjacent devices, "
                               "sharing the same physical parent composite device. Identical to calling query_sensors.")
        .def("first_depth_sensor", [](rs2::device& self) { return self.first<rs2::depth_sensor>(); }) // No docstring in C++
        .def("first_roi_sensor", [](rs2::device& self) { return self.first<rs2::roi_sensor>(); }) // No docstring in C++
        .def("first_pose_sensor", [](rs2::device& self) { return self.first<rs2::pose_sensor>(); }) // No docstring in C++
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
        .def(BIND_DOWNCAST(device, updatable))
        .def(BIND_DOWNCAST(device, update_device))
        .def("__repr__", [](const rs2::device &self) {
            std::stringstream ss;
            ss << "<" SNAME ".device: " << self.get_info(RS2_CAMERA_INFO_NAME)
                << " (S/N: " << self.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER)
                << ")>";
            return ss.str();
        });

    py::class_<rs2::debug_protocol> debug_protocol(m, "debug_protocol"); // No docstring in C++
    debug_protocol.def(py::init<rs2::device>())
        .def("send_and_receive_raw_data", &rs2::debug_protocol::send_and_receive_raw_data,
             "input"_a);  // No docstring in C++

    py::class_<rs2::updatable> updatable(m, "updatable");
    updatable.def(py::init<rs2::device>())
        .def("enter_update_state", &rs2::updatable::enter_update_state)
        .def("create_flash_backup", (std::vector<uint8_t>(rs2::updatable::*)() const) &rs2::updatable::create_flash_backup,
            "Create backup of camera flash memory. Such backup does not constitute valid firmware image, and cannot be "
            "loaded back to the device, but it does contain all calibration and device information.")
        .def("create_flash_backup", [](rs2::updatable& self, std::function<void(float)> f) { return self.create_flash_backup(f); },
            "Create backup of camera flash memory. Such backup does not constitute valid firmware image, and cannot be "
            "loaded back to the device, but it does contain all calibration and device information.",
            "callback"_a)
        .def("update_unsigned", [](rs2::updatable& self, const std::vector<uint8_t>& fw_image, int update_mode) { return self.update_unsigned(fw_image, update_mode); },
            "Update an updatable device to the provided unsigned firmware. this call is executed on the caller's thread", "fw_image"_a, "update_mode"_a = RS2_UNSIGNED_UPDATE_MODE_UPDATE)
        .def("update_unsigned", [](rs2::updatable& self, const std::vector<uint8_t>& fw_image, std::function<void(float)> f, int update_mode) { return self.update_unsigned(fw_image, f, update_mode); },
            "Update an updatable device to the provided unsigned firmware. this call is executed on the caller's thread and it supports progress notifications via the callback",
            "fw_image"_a, "callback"_a, "update_mode"_a = RS2_UNSIGNED_UPDATE_MODE_UPDATE);

    py::class_<rs2::update_device> update_device(m, "update_device");
    update_device.def(py::init<rs2::device>())
        .def("update", [](rs2::update_device& self, const std::vector<uint8_t>& fw_image) { return self.update(fw_image); },
            "Update an updatable device to the provided firmware. this call is executed on the caller's thread", "fw_image"_a)
        .def("update", [](rs2::update_device& self, const std::vector<uint8_t>& fw_image, std::function<void(float)> f) { return self.update(fw_image, f); },
            "Update an updatable device to the provided firmware. this call is executed on the caller's thread and it supports progress notifications via the callback",
            "fw_image"_a, "callback"_a);

    py::class_<rs2::device_list> device_list(m, "device_list"); // No docstring in C++
    device_list.def(py::init<>())
        .def("contains", &rs2::device_list::contains) // No docstring in C++
        .def("__getitem__", [](const rs2::device_list& self, size_t i) {
            if (i >= self.size())
                throw py::index_error();
            return self[uint32_t(i)];
        })
        .def("__len__", &rs2::device_list::size)
        .def("size", &rs2::device_list::size) // No docstring in C++
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
        .def("front", &rs2::device_list::front) // No docstring in C++
        .def("back", &rs2::device_list::back); // No docstring in C++

    // Not binding status_changed_callback, templated

    py::class_<rs2::event_information> event_information(m, "event_information"); // No docstring in C++
    event_information.def("was_removed", &rs2::event_information::was_removed, "Check if "
                          "a specific device was disconnected.", "dev"_a)
        .def("was_added", &rs2::event_information::was_added, "Check if "
             "a specific device was added.", "dev"_a)
        .def("get_new_devices", &rs2::event_information::get_new_devices, "Returns a "
             "list of all newly connected devices");

    py::class_<rs2::tm2, rs2::device> tm2(m, "tm2"); // No docstring in C++
    tm2.def(py::init<rs2::device>(), "device"_a)
        .def("enable_loopback", &rs2::tm2::enable_loopback, "Enter the given device into "
             "loopback operation mode that uses the given file as input for raw data", "filename"_a)
        .def("disable_loopback", &rs2::tm2::disable_loopback, "Restores the given device into normal operation mode")
        .def("is_loopback_enabled", &rs2::tm2::is_loopback_enabled, "Checks if the device is in loopback mode or not")
        .def("connect_controller", &rs2::tm2::connect_controller, "Connects to a given tm2 controller", "mac_address"_a)
        .def("disconnect_controller", &rs2::tm2::disconnect_controller, "Disconnects a given tm2 controller", "id"_a);


    /* rs_frame.hpp */
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

    py::class_<rs2::frame> frame(m, "frame", "Base class for multiple frame extensions");
    frame.def(py::init<>())
        // .def(py::self = py::self) // can't overload assignment in python
        .def(py::init<rs2::frame>())
        .def("swap", &rs2::frame::swap, "Swap the internal frame handle with the one in parameter", "other"_a)
        .def("__nonzero__", &rs2::frame::operator bool, "check if internal frame handle is valid")
        .def("get_timestamp", &rs2::frame::get_timestamp, "Retrieve the time at which the frame was captured")
        .def_property_readonly("timestamp", &rs2::frame::get_timestamp, "Time at which the frame was captured. Identical to calling get_timestamp.")
        .def("get_frame_timestamp_domain", &rs2::frame::get_frame_timestamp_domain, "Retrieve the timestamp domain.")
        .def_property_readonly("frame_timestamp_domain", &rs2::frame::get_frame_timestamp_domain, "The timestamp domain. Identical to calling get_frame_timestamp_domain.")
        .def("get_frame_metadata", &rs2::frame::get_frame_metadata, "Retrieve the current value of a single frame_metadata.", "frame_metadata"_a)
        .def("supports_frame_metadata", &rs2::frame::supports_frame_metadata, "Determine if the device allows a specific metadata to be queried.", "frame_metadata"_a)
        .def("get_frame_number", &rs2::frame::get_frame_number, "Retrieve the frame number.")
        .def_property_readonly("frame_number", &rs2::frame::get_frame_number, "The frame number. Identical to calling get_frame_number.")
        .def("get_data", get_frame_data, "Retrieve data from the frame handle.", py::keep_alive<0, 1>())
        .def_property_readonly("data", get_frame_data, "Data from the frame handle. Identical to calling get_data.", py::keep_alive<0, 1>())
        .def("get_profile", &rs2::frame::get_profile, "Retrieve stream profile from frame handle.")
        .def_property_readonly("profile", &rs2::frame::get_profile, "Stream profile from frame handle. Identical to calling get_profile.")
        .def("keep", &rs2::frame::keep, "Keep the frame, otherwise if no refernce to the frame, the frame will be released.")
        .def(BIND_DOWNCAST(frame, frame))
        .def(BIND_DOWNCAST(frame, points))
        .def(BIND_DOWNCAST(frame, frameset))
        .def(BIND_DOWNCAST(frame, video_frame))
        .def(BIND_DOWNCAST(frame, depth_frame))
        .def(BIND_DOWNCAST(frame, motion_frame))
        .def(BIND_DOWNCAST(frame, pose_frame));
        // No apply_filter?

    py::class_<rs2::video_frame, rs2::frame> video_frame(m, "video_frame", "Extends the frame class with additional video related attributes and functions.");
    video_frame.def(py::init<rs2::frame>())
        .def("get_width", &rs2::video_frame::get_width, "Returns image width in pixels.")
        .def_property_readonly("width", &rs2::video_frame::get_width, "Image width in pixels. Identical to calling get_width.")
        .def("get_height", &rs2::video_frame::get_height, "Returns image height in pixels.")
        .def_property_readonly("height", &rs2::video_frame::get_height, "Image height in pixels. Identical to calling get_height.")
        .def("get_stride_in_bytes", &rs2::video_frame::get_stride_in_bytes, "Retrieve frame stride, meaning the actual line width in memory in bytes (not the logical image width).")
        .def_property_readonly("stride_in_bytes", &rs2::video_frame::get_stride_in_bytes, "Frame stride, meaning the actual line width in memory in bytes (not the logical image width). Identical to calling get_stride_in_bytes.")
        .def("get_bits_per_pixel", &rs2::video_frame::get_bits_per_pixel, "Retrieve bits per pixel.")
        .def_property_readonly("bits_per_pixel", &rs2::video_frame::get_bits_per_pixel, "Bits per pixel. Identical to calling get_bits_per_pixel.")
        .def("get_bytes_per_pixel", &rs2::video_frame::get_bytes_per_pixel, "Retrieve bytes per pixel.")
        .def_property_readonly("bytes_per_pixel", &rs2::video_frame::get_bytes_per_pixel, "Bytes per pixel. Identical to calling get_bytes_per_pixel.");


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
        .def_readwrite("translation",           &rs2_pose::translation, "X, Y, Z values of translation, in meters (relative to initial position)")
        .def_readwrite("velocity",              &rs2_pose::velocity, "X, Y, Z values of velocity, in meters/sec")
        .def_readwrite("acceleration",          &rs2_pose::acceleration, "X, Y, Z values of acceleration, in meters/sec^2")
        .def_readwrite("rotation",              &rs2_pose::rotation, "Qi, Qj, Qk, Qr components of rotation as represented in quaternion rotation (relative to initial position)")
        .def_readwrite("angular_velocity",      &rs2_pose::angular_velocity, "X, Y, Z values of angular velocity, in radians/sec")
        .def_readwrite("angular_acceleration",  &rs2_pose::angular_acceleration, "X, Y, Z values of angular acceleration, in radians/sec^2")
        .def_readwrite("tracker_confidence",    &rs2_pose::tracker_confidence, "Pose confidence 0x0 - Failed, 0x1 - Low, 0x2 - Medium, 0x3 - High")
        .def_readwrite("mapper_confidence",     &rs2_pose::mapper_confidence, "Pose map confidence 0x0 - Failed, 0x1 - Low, 0x2 - Medium, 0x3 - High");

    py::class_<rs2::motion_frame, rs2::frame> motion_frame(m, "motion_frame", "Extends the frame class with additional motion related attributes and functions");
    motion_frame.def(py::init<rs2::frame>())
        .def("get_motion_data", &rs2::motion_frame::get_motion_data, "Retrieve the motion data from IMU sensor.")
        .def_property_readonly("motion_data", &rs2::motion_frame::get_motion_data, "Motion data from IMU sensor. Identical to calling get_motion_data.");

    py::class_<rs2::pose_frame, rs2::frame> pose_frame(m, "pose_frame", "Extends the frame class with additional pose related attributes and functions.");
    pose_frame.def(py::init<rs2::frame>())
        .def("get_pose_data", &rs2::pose_frame::get_pose_data, "Retrieve the pose data from T2xx position tracking sensor.")
        .def_property_readonly("pose_data", &rs2::pose_frame::get_pose_data, "Pose data from T2xx position tracking sensor. Identical to calling get_pose_data.");

    py::class_<rs2::vertex> vertex(m, "vertex"); // No docstring in C++
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

    py::class_<rs2::texture_coordinate> texture_coordinate(m, "texture_coordinate"); // No docstring in C++
    texture_coordinate.def_readwrite("u", &rs2::texture_coordinate::u)
        .def_readwrite("v", &rs2::texture_coordinate::v)
        .def(py::init([]() { return rs2::texture_coordinate{}; }))
        .def(py::init([](float u, float v) { return rs2::texture_coordinate{ u, v }; }))
        .def("__repr__", [](const rs2::texture_coordinate& t) {
            std::ostringstream oss;
            oss << t.u << ", " << t.v;
            return oss.str();
        });

    py::class_<rs2::points, rs2::frame> points(m, "points", "Extends the frame class with additional point cloud related attributes and functions.");
    points.def(py::init<>())
        .def(py::init<rs2::frame>())
        .def("get_vertices", [](rs2::points& self, int dims) {
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
        }, "Retrieve the vertices of the point cloud", py::keep_alive<0, 1>(), "dims"_a=1)
        .def("get_texture_coordinates", [](rs2::points& self, int dims) {
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
        }, "Retrieve the texture coordinates (uv map) for the point cloud", py::keep_alive<0, 1>(), "dims"_a=1)
        .def("export_to_ply", &rs2::points::export_to_ply, "Export the point cloud to a PLY file")
        .def("size", &rs2::points::size); // No docstring in C++

    // TODO: Deprecate composite_frame, replace with frameset
    py::class_<rs2::frameset, rs2::frame> frameset(m, "composite_frame", "Extends the frame class with additional frameset related attributes and functions");
    frameset.def(py::init<rs2::frame>())
        .def("first_or_default", &rs2::frameset::first_or_default, "Retrieve the first frame of a specific stream and optionally with a specific format. "
             "If no frame is found, return an empty frame instance.", "s"_a, "f"_a = RS2_FORMAT_ANY)
        .def("first", &rs2::frameset::first, "Retrieve the first frame of a specific stream type and optionally with a specific format. "
             "If no frame is found, an error will be thrown.", "s"_a, "f"_a = RS2_FORMAT_ANY)
        .def("size", &rs2::frameset::size, "Return the size of the frameset")
        .def("__len__", &rs2::frameset::size, "Return the size of the frameset")
        .def("foreach", [](const rs2::frameset& self, std::function<void(rs2::frame)> callable) {
            self.foreach(callable);
        }, "Extract internal frame handles from the frameset and invoke the action function", "callable"_a)
        .def("__getitem__", &rs2::frameset::operator[])
        .def("get_depth_frame", &rs2::frameset::get_depth_frame, "Retrieve the first depth frame, if no frame is found, return an empty frame instance.")
        .def("get_color_frame", &rs2::frameset::get_color_frame, "Retrieve the first color frame, if no frame is found, search for the color frame from IR stream. "
             "If one still can't be found, return an empty frame instance.")
        .def("get_infrared_frame", &rs2::frameset::get_infrared_frame, "Retrieve the first infrared frame, if no frame is "
             "found, return an empty frame instance.", "index"_a = 0)
        .def("get_fisheye_frame", &rs2::frameset::get_fisheye_frame, "Retrieve the fisheye monochrome video frame", "index"_a=0)
        .def("get_pose_frame", &rs2::frameset::get_pose_frame, "Retrieve the pose frame", "index"_a = 0)
        .def("__iter__", [](rs2::frameset& self) {
            return py::make_iterator(self.begin(), self.end());
        }, py::keep_alive<0, 1>())
        .def("__getitem__", [](const rs2::frameset& self, py::slice slice) {
            size_t start, stop, step, slicelength;
            if (!slice.compute(self.size(), &start, &stop, &step, &slicelength))
                throw py::error_already_set();
            auto *flist = new std::vector<rs2::frame>(slicelength);
            for (size_t i = 0; i < slicelength; ++i) {
                (*flist)[i] = self[start];
                start += step;
            }
            return flist;
        });

    py::class_<rs2::depth_frame, rs2::video_frame> depth_frame(m, "depth_frame", "Extends the video_frame class with additional depth related attributes and functions.");
    depth_frame.def(py::init<rs2::frame>())
        .def("get_distance", &rs2::depth_frame::get_distance, "x"_a, "y"_a, "Provide the depth in meters at the given pixel");

    /* rs2_processing.hpp */
    py::class_<rs2::filter_interface> filter_interface(m, "filter_interface", "Interface for frame filtering functionality");
    filter_interface.def("process", &rs2::filter_interface::process, "frame"_a); // No docstring in C++


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

    /* rs2_processing.hpp */
    py::class_<rs2::frame_source> frame_source(m, "frame_source", "The source used to generate frames, which is usually done by the low level driver for each sensor. "
                                               "frame_source is one of the parameters of processing_block's callback function, which can be used to re-generate the "
                                               "frame and via frame_ready invoke another callback function to notify application frame is ready.");
    frame_source.def("allocate_video_frame", &rs2::frame_source::allocate_video_frame, "Allocate a new video frame with given params"
                     "profile"_a, "original"_a, "new_bpp"_a = 0, "new_width"_a = 0,
                     "new_height"_a = 0, "new_stride"_a = 0, "frame_type"_a = RS2_EXTENSION_VIDEO_FRAME)
        .def("allocate_points", &rs2::frame_source::allocate_points, "profile"_a,
             "original"_a) // No docstring in C++
        .def("allocate_composite_frame", &rs2::frame_source::allocate_composite_frame,
             "Allocate composite frame with given params", "frames"_a)
        .def("frame_ready", &rs2::frame_source::frame_ready, "Invoke the "
             "callback funtion informing the frame is ready.", "result"_a);

    py::class_<rs2::frame_queue> frame_queue(m, "frame_queue", "Frame queues are the simplest cross-platform "
                                             "synchronization primitive provided by librealsense to help "
                                             "developers who are not using async APIs.");
    frame_queue.def(py::init<unsigned int>())
        .def(py::init<>())
        .def("enqueue", &rs2::frame_queue::enqueue, "Enqueue a new frame into the queue.", "f"_a)
        .def("wait_for_frame", &rs2::frame_queue::wait_for_frame, "Wait until a new frame "
             "becomes available in the queue and dequeue it.", "timeout_ms"_a = 5000, py::call_guard<py::gil_scoped_release>())
        .def("poll_for_frame", [](const rs2::frame_queue &self) {
            rs2::frame frame;
            self.poll_for_frame(&frame);
            return frame;
        }, "Poll if a new frame is available and dequeue it if it is")
        .def("try_wait_for_frame", [](const rs2::frame_queue &self, unsigned int timeout_ms) {
            rs2::frame frame;
            auto success = self.try_wait_for_frame(&frame, timeout_ms);
            return std::make_tuple(success, frame);
        }, "timeout_ms"_a=5000, py::call_guard<py::gil_scoped_release>()) // No docstring in C++
        .def("__call__", &rs2::frame_queue::operator(), "Identical to calling enqueue", "f"_a)
        .def("capacity", &rs2::frame_queue::capacity, "Return the capacity of the queue");

    // Not binding frame_processor_callback, templated

    py::class_<rs2::processing_block, rs2::options> processing_block(m, "processing_block", "Define the processing block workflow, inherit this class to "
                                                                     "generate your own processing_block.");
    processing_block.def(py::init([](std::function<void(rs2::frame, rs2::frame_source&)> processing_function) {
            return new rs2::processing_block(processing_function);
        }), "processing_function"_a)
        .def("start", [](rs2::processing_block& self, std::function<void(rs2::frame)> f) {
            self.start(f);
        }, "Start the processing block with callback function to inform the application the frame is processed.", "callback"_a)
        .def("invoke", &rs2::processing_block::invoke, "Ask processing block to process the frame", "f"_a)
        .def("supports", (bool (rs2::processing_block::*)(rs2_camera_info) const) &rs2::processing_block::supports, "Check if a specific camera info field is supported.")
        .def("get_info", &rs2::processing_block::get_info, "Retrieve camera specific information, like versions of various internal components.");
        /*.def("__call__", &rs2::processing_block::operator(), "f"_a)*/
        // supports(camera_info) / get_info(camera_info)?

    py::class_<rs2::filter, rs2::processing_block, rs2::filter_interface> filter(m, "filter", "Define the filter workflow, inherit this class to generate your own filter.");
    filter.def(py::init([](std::function<void(rs2::frame, rs2::frame_source&)> filter_function, int queue_size) {
            return new rs2::filter(filter_function, queue_size);
    }), "filter_function"_a, "queue_size"_a = 1)
        .def(BIND_DOWNCAST(filter, decimation_filter))
        .def(BIND_DOWNCAST(filter, disparity_transform))
        .def(BIND_DOWNCAST(filter, hole_filling_filter))
        .def(BIND_DOWNCAST(filter, spatial_filter))
        .def(BIND_DOWNCAST(filter, temporal_filter))
        .def(BIND_DOWNCAST(filter, threshold_filter))
        .def(BIND_DOWNCAST(filter, zero_order_invalidation))
        .def("__nonzero__", &rs2::filter::operator bool); // No docstring in C++
    // get_queue?
    // is/as?


    // Not binding syncer_processing_block, not in Python API

    py::class_<rs2::pointcloud, rs2::filter> pointcloud(m, "pointcloud", "Generates 3D point clouds based on a depth frame. Can also map textures from a color frame.");
    pointcloud.def(py::init<>())
        .def(py::init<rs2_stream, int>(), "stream"_a, "index"_a = 0)
        .def("calculate", &rs2::pointcloud::calculate, "Generate the pointcloud and texture mappings of depth map.", "depth"_a)
        .def("map_to", &rs2::pointcloud::map_to, "Map the point cloud to the given color frame.", "mapped"_a);

    py::class_<rs2::syncer> syncer(m, "syncer", "Sync instance to align frames from different streams");
    syncer.def(py::init<int>(), "queue_size"_a = 1)
        .def("wait_for_frames", &rs2::syncer::wait_for_frames, "Wait until a coherent set "
            "of frames becomes available", "timeout_ms"_a = 5000, py::call_guard<py::gil_scoped_release>())
        .def("poll_for_frames", [](const rs2::syncer &self) {
            rs2::frameset frames;
            self.poll_for_frames(&frames);
            return frames;
        }, "Check if a coherent set of frames is available")
        .def("try_wait_for_frames", [](const rs2::syncer &self, unsigned int timeout_ms) {
            rs2::frameset fs;
            auto success = self.try_wait_for_frames(&fs, timeout_ms);
            return std::make_tuple(success, fs);
        }, "timeout_ms"_a = 5000, py::call_guard<py::gil_scoped_release>()); // No docstring in C++
        /*.def("__call__", &rs2::syncer::operator(), "frame"_a)*/

    py::class_<rs2::threshold_filter, rs2::filter> threshold(m, "threshold_filter", "Depth thresholding filter. By controlling min and "
                                                             "max options on the block, one could filter out depth values that are either too large "
                                                             "or too small, as a software post-processing step");
    threshold.def(py::init<float, float>(), "min_dist"_a=0.15f, "max_dist"_a=4.f);

	py::class_<rs2::units_transform, rs2::filter> units_transform(m, "units_transform");
	units_transform.def(py::init<>());

    py::class_<rs2::colorizer, rs2::filter> colorizer(m, "colorizer", "Colorizer filter generates color images based on input depth frame");
    colorizer.def(py::init<>())
        .def(py::init<float>(), "Possible values for color_scheme:\n"
             "0 - Jet\n"
             "1 - Classic\n"
             "2 - WhiteToBlack\n"
             "3 - BlackToWhite\n"
             "4 - Bio\n"
             "5 - Cold\n"
             "6 - Warm\n"
             "7 - Quantized\n"
             "8 - Pattern", "color_scheme"_a)
        .def("colorize", &rs2::colorizer::colorize, "Start to generate color image base on depth frame", "depth"_a)
        /*.def("__call__", &rs2::colorizer::operator())*/;

    py::class_<rs2::align, rs2::filter> align(m, "align", "Performs alignment between depth image and another image.");
    align.def(py::init<rs2_stream>(), "To perform alignment of a depth image to the other, set the align_to parameter with the other stream type.\n"
              "To perform alignment of a non depth image to a depth image, set the align_to parameter to RS2_STREAM_DEPTH.\n"
              "Camera calibration and frame's stream type are determined on the fly, according to the first valid frameset passed to process().", "align_to"_a)
        .def("process", (rs2::frameset (rs2::align::*)(rs2::frameset)) &rs2::align::process, "Run thealignment process on the given frames to get an aligned set of frames", "frames"_a);

    py::class_<rs2::decimation_filter, rs2::filter> decimation_filter(m, "decimation_filter", "Performs downsampling by using the median with specific kernel size.");
    decimation_filter.def(py::init<>())
        .def(py::init<float>(), "magnitude"_a);

    py::class_<rs2::temporal_filter, rs2::filter> temporal_filter(m, "temporal_filter", "Temporal filter smooths the image by calculating multiple frames "
                                                                  "with alpha and delta settings. Alpha defines the weight of current frame, and delta defines the"
                                                                  "threshold for edge classification and preserving.");
    temporal_filter.def(py::init<>())
        .def(py::init<float, float, int>(), "Possible values for persistence_control:\n"
             "1 - Valid in 8/8 - Persistency activated if the pixel was valid in 8 out of the last 8 frames\n"
             "2 - Valid in 2 / last 3 - Activated if the pixel was valid in two out of the last 3 frames\n"
             "3 - Valid in 2 / last 4 - Activated if the pixel was valid in two out of the last 4 frames\n"
             "4 - Valid in 2 / 8 - Activated if the pixel was valid in two out of the last 8 frames\n"
             "5 - Valid in 1 / last 2 - Activated if the pixel was valid in one of the last two frames\n"
             "6 - Valid in 1 / last 5 - Activated if the pixel was valid in one out of the last 5 frames\n"
             "7 - Valid in 1 / last 8 - Activated if the pixel was valid in one out of the last 8 frames\n"
             "8 - Persist Indefinitely - Persistency will be imposed regardless of the stored history(most aggressive filtering)",
             "smooth_alpha"_a, "smooth_delta"_a, "persistence_control"_a);

    py::class_<rs2::spatial_filter, rs2::filter> spatial_filter(m, "spatial_filter", "Spatial filter smooths the image by calculating frame with alpha and delta settings. "
                                                                "Alpha defines the weight of the current pixel for smoothing, and is bounded within [25..100]%. Delta "
                                                                "defines the depth gradient below which the smoothing will occur as number of depth levels.");
    spatial_filter.def(py::init<>())
        .def(py::init<float, float, float, float>(), "smooth_alpha"_a, "smooth_delta"_a, "magnitude"_a, "hole_fill"_a);;

    py::class_<rs2::hole_filling_filter, rs2::filter> hole_filling_filter(m, "hole_filling_filter", "The processing performed depends on the selected hole filling mode.");
    hole_filling_filter.def(py::init<>())
        .def(py::init<int>(), "Possible values for mode:\n"
             "0 - fill_from_left - Use the value from the left neighbor pixel to fill the hole\n"
             "1 - farest_from_around - Use the value from the neighboring pixel which is furthest away from the sensor\n"
             "2 - nearest_from_around - -Use the value from the neighboring pixel closest to the sensor", "mode"_a);

    py::class_<rs2::disparity_transform, rs2::filter> disparity_transform(m, "disparity_transform", "Converts from depth representation "
                                                                          "to disparity representation and vice - versa in depth frames");
    disparity_transform.def(py::init<bool>(), "transform_to_disparity"_a=true);

    py::class_<rs2::yuy_decoder, rs2::filter> yuy_decoder(m, "yuy_decoder", "Converts frames in raw YUY format to RGB. This conversion is somewhat costly, "
                                                          "but the SDK will automatically try to use SSE2, AVX, or CUDA instructions where available to "
                                                          "get better performance. Othere implementations (GLSL, OpenCL, Neon, NCS) should follow.");
    yuy_decoder.def(py::init<>());

    py::class_<rs2::zero_order_invalidation, rs2::filter> zero_order_invalidation(m, "zero_order_invalidation", "Fixes the zero order artifact");
    zero_order_invalidation.def(py::init<>());

    /* rs_export.hpp */
    // py::class_<rs2::save_to_ply, rs2::filter> save_to_ply(m, "save_to_ply"); // No docstring in C++
    // save_to_ply.def(py::init<std::string, rs2::pointcloud>(), "filename"_a = "RealSense Pointcloud ", "pc"_a = rs2::pointcloud())
    //     .def_readonly_static("option_ignore_color", &rs2::save_to_ply::OPTION_IGNORE_COLOR);

    py::class_<rs2::save_single_frameset, rs2::filter> save_single_frameset(m, "save_single_frameset"); // No docstring in C++
    save_single_frameset.def(py::init<std::string>(), "filename"_a = "RealSense Frameset ");

    /* rs2_record_playback.hpp */
    py::class_<rs2::playback, rs2::device> playback(m, "playback"); // No docstring in C++
    playback.def(py::init<rs2::device>(), "device"_a)
        .def("pause", &rs2::playback::pause, "Pauses the playback. Calling pause() in \"Paused\" status does nothing. If "
             "pause() is called while playback status is \"Playing\" or \"Stopped\", the playback will not play until resume() is called.")
        .def("resume", &rs2::playback::resume, "Un-pauses the playback. Calling resume() while playback status is \"Playing\" or \"Stopped\" does nothing.")
        .def("file_name", &rs2::playback::file_name, "The name of the playback file.")
        .def("get_position", &rs2::playback::get_position, "Retrieves the current position of the playback in the file in terms of time. Units are expressed in nanoseconds.")
        .def("get_duration", &rs2::playback::get_duration, "Retrieves the total duration of the file.")
        .def("seek", &rs2::playback::seek, "Sets the playback to a specified time point of the played data." "time"_a)
        .def("is_real_time", &rs2::playback::is_real_time, "Indicates if playback is in real time mode or non real time.")
        .def("set_real_time", &rs2::playback::set_real_time, "Set the playback to work in real time or non real time. In real time mode, playback will "
             "play the same way the file was recorded. If the application takes too long to handle the callback, frames may be dropped. In non real time "
             "mode, playback will wait for each callback to finish handling the data before reading the next frame. In this mode no frames will be dropped, "
             "and the application controls the framerate of playback via callback duration." "real_time"_a)
        // set_playback_speed?
        .def("set_status_changed_callback", [](rs2::playback& self, std::function<void(rs2_playback_status)> callback) {
            self.set_status_changed_callback(callback);
        }, "Register to receive callback from playback device upon its status changes. Callbacks are invoked from the reading thread, "
           "and as such any heavy processing in the callback handler will affect the reading thread and may cause frame drops/high latency.", "callback"_a)
        .def("current_status", &rs2::playback::current_status, "Returns the current state of the playback device");
        // Stop?

    py::class_<rs2::recorder, rs2::device> recorder(m, "recorder", "Records the given device and saves it to the given file as rosbag format.");
    recorder.def(py::init<const std::string&, rs2::device>())
        .def("pause", &rs2::recorder::pause, "Pause the recording device without stopping the actual device from streaming.")
        .def("resume", &rs2::recorder::resume, "Unpauses the recording device, making it resume recording.");
        // filename?

    /* rs2_sensor.hpp */
    py::class_<rs2::stream_profile> stream_profile(m, "stream_profile", "Stores details about the profile of a stream.");
    stream_profile.def(py::init<>())
        .def("stream_index", &rs2::stream_profile::stream_index, "The stream's index")
        .def("stream_type", &rs2::stream_profile::stream_type, "The stream's type")
        .def("format", &rs2::stream_profile::format, "The stream's format")
        .def("fps", &rs2::stream_profile::fps, "The streams framerate")
        .def("unique_id", &rs2::stream_profile::unique_id, "Unique index assigned whent the stream was created")
        .def("clone", &rs2::stream_profile::clone, "Clone the current profile and change the type, index and format to input parameters", "type"_a, "index"_a, "format"_a)
        .def(BIND_DOWNCAST(stream_profile, stream_profile))
        .def(BIND_DOWNCAST(stream_profile, video_stream_profile))
        .def(BIND_DOWNCAST(stream_profile, motion_stream_profile))
        .def("stream_name", &rs2::stream_profile::stream_name, "The stream's human-readable name.")
        .def("is_default", &rs2::stream_profile::is_default, "Checks if the stream profile is marked/assigned as default, "
             "meaning that the profile will be selected when the user requests stream configuration using wildcards.")
        .def("__nonzero__", &rs2::stream_profile::operator bool, "Checks if the profile is valid")
        .def("get_extrinsics_to", &rs2::stream_profile::get_extrinsics_to, "Get the extrinsic transformation between two profiles (representing physical sensors)", "to"_a)
        .def("register_extrinsics_to", &rs2::stream_profile::register_extrinsics_to, "Assign extrinsic transformation parameters "
             "to a specific profile (sensor). The extrinsic information is generally available as part of the camera calibration, "
             "and librealsense is responsible for retrieving and assigning these parameters where appropriate. This specific function "
             "is intended for synthetic/mock-up (software) devices for which the parameters are produced and injected by the user.", "to"_a, "extrinsics"_a)
        .def("__repr__", [](const rs2::stream_profile& self) {
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

    py::class_<rs2::video_stream_profile, rs2::stream_profile> video_stream_profile(m, "video_stream_profile", "Stream profile instance which contains additional video attributes.");
    video_stream_profile.def(py::init<const rs2::stream_profile&>(), "sp"_a)
        .def("width", &rs2::video_stream_profile::width) // No docstring in C++
        .def("height", &rs2::video_stream_profile::height) // No docstring in C++
        .def("get_intrinsics", &rs2::video_stream_profile::get_intrinsics, "Get stream profile instrinsics attributes.")
        .def_property_readonly("intrinsics", &rs2::video_stream_profile::get_intrinsics, "Stream profile instrinsics attributes. Identical to calling get_intrinsics.")
        .def("__repr__", [](const rs2::video_stream_profile& self) {
            std::stringstream ss;
            ss << "<" SNAME ".video_stream_profile: "
                << self.stream_type() << "(" << self.stream_index() << ") " << self.width()
                << "x" << self.height() << " @ " << self.fps() << "fps "
                << self.format() << ">";
            return ss.str();
        });

    py::class_<rs2::motion_stream_profile, rs2::stream_profile> motion_stream_profile(m, "motion_stream_profile", "Stream profile instance which contains IMU-specific intrinsics.");
    motion_stream_profile.def(py::init<const rs2::stream_profile&>(), "sp"_a)
        .def("get_motion_intrinsics", &rs2::motion_stream_profile::get_motion_intrinsics, "Returns scale and bias of a motion stream.");

    py::class_<rs2::pose_stream_profile, rs2::stream_profile> pose_stream_profile(m, "pose_stream_profile", "Stream profile instance with an explicit pose extension type.");
    pose_stream_profile.def(py::init<const rs2::stream_profile&>(), "sp"_a);

    py::class_<rs2::notification> notification(m, "notification"); // No docstring in C++
    notification.def(py::init<>())
        .def("get_category", &rs2::notification::get_category,
             "Retrieve the notification's category.")
        .def_property_readonly("category", &rs2::notification::get_category,
                               "The notification's category. Identical to calling get_category.")
        .def("get_description", &rs2::notification::get_description,
             "Retrieve the notification's description.")
        .def_property_readonly("description", &rs2::notification::get_description,
                               "The notification's description. Identical to calling get_description.")
        .def("get_timestamp", &rs2::notification::get_timestamp,
             "Retrieve the notification's arrival timestamp.")
        .def_property_readonly("timestamp", &rs2::notification::get_timestamp,
                               "The notification's arrival timestamp. Identical to calling get_timestamp.")
        .def("get_severity", &rs2::notification::get_severity,
             "Retrieve the notification's severity.")
        .def_property_readonly("severity", &rs2::notification::get_severity,
                               "The notification's severity. Identical to calling get_severity.")
        .def("get_serialized_data", &rs2::notification::get_severity,
             "Retrieve the notification's serialized data.")
        .def_property_readonly("serialized_data", &rs2::notification::get_serialized_data,
                               "The notification's serialized data. Identical to calling get_serialized_data.")
        .def("__repr__", [](const rs2::notification &n) {
            return n.get_description();
        });

    // not binding notifications_callback, templated
    py::class_<rs2::sensor, rs2::options> sensor(m, "sensor"); // No docstring in C++
    sensor.def("open", (void (rs2::sensor::*)(const rs2::stream_profile&) const) &rs2::sensor::open,
               "Open sensor for exclusive access, by commiting to a configuration", "profile"_a)
        .def("supports", (bool (rs2::sensor::*)(rs2_camera_info) const) &rs2::sensor::supports,
             "Check if specific camera info is supported.", "info")
        .def("supports", (bool (rs2::sensor::*)(rs2_option) const) &rs2::options::supports,
             "Check if specific camera info is supported.", "info")
        .def("get_info", &rs2::sensor::get_info, "Retrieve camera specific information, "
             "like versions of various internal components.", "info"_a)
        .def("set_notifications_callback", [](const rs2::sensor& self, std::function<void(rs2::notification)> callback) {
            self.set_notifications_callback(callback);
        }, "Register Notifications callback", "callback"_a)
        .def("open", (void (rs2::sensor::*)(const std::vector<rs2::stream_profile>&) const) &rs2::sensor::open,
             "Open sensor for exclusive access, by committing to a composite configuration, specifying one or "
             "more stream profiles.", "profiles"_a)
        .def("close", &rs2::sensor::close, "Close sensor for exclusive access.", py::call_guard<py::gil_scoped_release>())
        .def("start", [](const rs2::sensor& self, std::function<void(rs2::frame)> callback) {
            self.start(callback);
        }, "Start passing frames into user provided callback.", "callback"_a)
        .def("start", [](const rs2::sensor& self, rs2::syncer syncer) {
            self.start(syncer);
        }, "Start passing frames into user provided syncer.", "callback"_a)
        .def("start", [](const rs2::sensor& self, rs2::frame_queue& queue) {
            self.start(queue);
        }, "start passing frames into specified frame_queue", "queue"_a)
        .def("stop", &rs2::sensor::stop, "Stop streaming.", py::call_guard<py::gil_scoped_release>())
        .def("get_stream_profiles", &rs2::sensor::get_stream_profiles, "Retrieves the list of stream profiles supported by the sensor.")
        .def_property_readonly("profiles", &rs2::sensor::get_stream_profiles, "The list of stream profiles supported by the sensor. Identical to calling get_stream_profiles")
        .def("get_recommended_filters", &rs2::sensor::get_recommended_filters, "Return the recommended list of filters by the sensor.")
        .def(py::init<>())
        .def("__nonzero__", &rs2::sensor::operator bool) // No docstring in C++
        .def(BIND_DOWNCAST(sensor, roi_sensor))
        .def(BIND_DOWNCAST(sensor, depth_sensor))
        .def(BIND_DOWNCAST(sensor, pose_sensor))
        .def(BIND_DOWNCAST(sensor, wheel_odometer));

    py::class_<rs2::roi_sensor, rs2::sensor> roi_sensor(m, "roi_sensor"); // No docstring in C++
    roi_sensor.def(py::init<rs2::sensor>(), "sensor"_a)
        .def("set_region_of_interest", &rs2::roi_sensor::set_region_of_interest, "roi"_a) // No docstring in C++
        .def("get_region_of_interest", &rs2::roi_sensor::get_region_of_interest) // No docstring in C++
        .def("__nonzero__", &rs2::roi_sensor::operator bool); // No docstring in C++

    py::class_<rs2::depth_sensor, rs2::sensor> depth_sensor(m, "depth_sensor"); // No docstring in C++
    depth_sensor.def(py::init<rs2::sensor>(), "sensor"_a)
        .def("get_depth_scale", &rs2::depth_sensor::get_depth_scale,
             "Retrieves mapping between the units of the depth image and meters.")
        .def("__nonzero__", &rs2::depth_sensor::operator bool); // No docstring in C++

    py::class_<rs2::pose_sensor, rs2::sensor> pose_sensor(m, "pose_sensor"); // No docstring in C++
    pose_sensor.def(py::init<rs2::sensor>(), "sensor"_a)
        .def("import_localization_map", &rs2::pose_sensor::import_localization_map,
             "Load SLAM localization map from host to device.", "lmap_buf"_a)
        .def("export_localization_map", &rs2::pose_sensor::export_localization_map,
             "Extract SLAM localization map from device and store on host.")
        .def("set_static_node", &rs2::pose_sensor::set_static_node,
             "Create a named reference frame anchored to a specific 3D pose.")
        .def("get_static_node", &rs2::pose_sensor::get_static_node,
             "Retrieve a named reference frame anchored to a specific 3D pose.")
        .def("__nonzero__", &rs2::pose_sensor::operator bool); // No docstring in C++

    py::class_<rs2::wheel_odometer, rs2::sensor> wheel_odometer(m, "wheel_odometer"); // No docstring in C++
    wheel_odometer.def(py::init<rs2::sensor>(), "sensor"_a)
        .def("load_wheel_odometery_config", &rs2::wheel_odometer::load_wheel_odometery_config,
             "Load Wheel odometer settings from host to device.", "odometry_config_buf"_a)
        .def("send_wheel_odometry", &rs2::wheel_odometer::send_wheel_odometry,
             "Send wheel odometry data for each individual sensor (wheel)",
             "wo_sensor_id"_a, "frame_num"_a, "translational_velocity"_a)
        .def("__nonzero__", &rs2::wheel_odometer::operator bool);

    /* rs2_pipeline.hpp */
    py::class_<rs2::pipeline> pipeline(m, "pipeline", "The pipeline simplifies the user interaction with the device and computer vision processing modules.\n"
                                       "The class abstracts the camera configuration and streaming, and the vision modules triggering and threading.\n"
                                       "It lets the application focus on the computer vision output of the modules, or the device output data.\n"
                                       "The pipeline can manage computer vision modules, which are implemented as a processing blocks.\n"
                                       "The pipeline is the consumer of the processing block interface, while the application consumes the computer vision interface.");
    pipeline.def(py::init<rs2::context>(), "The caller can provide a context created by the application, usually for playback or testing purposes.", "ctx"_a = rs2::context())
        // TODO: Streamline this wall of text
        .def("start", (rs2::pipeline_profile(rs2::pipeline::*)(const rs2::config&)) &rs2::pipeline::start, "Start the pipeline streaming according to the configuraion.\n"
             "The pipeline streaming loop captures samples from the device, and delivers them to the attached computer vision modules and processing blocks, according to "
             "each module requirements and threading model.\n"
             "During the loop execution, the application can access the camera streams by calling wait_for_frames() or poll_for_frames().\n"
             "The streaming loop runs until the pipeline is stopped.\n"
             "Starting the pipeline is possible only when it is not started. If the pipeline was started, an exception is raised.\n"
             "The pipeline selects and activates the device upon start, according to configuration or a default configuration.\n"
             "When the rs2::config is provided to the method, the pipeline tries to activate the config resolve() result.\n"
             "If the application requests are conflicting with pipeline computer vision modules or no matching device is available on the platform, the method fails.\n"
             "Available configurations and devices may change between config resolve() call and pipeline start, in case devices are connected or disconnected, or another "
             "application acquires ownership of a device.", "config"_a)
        .def("start", (rs2::pipeline_profile(rs2::pipeline::*)()) &rs2::pipeline::start, "Start the pipeline streaming with its default configuration.\n"
             "The pipeline streaming loop captures samples from the device, and delivers them to the attached computer vision modules and processing "
             "blocks, according to each module requirements and threading model.\n"
             "During the loop execution, the application can access the camera streams by calling wait_for_frames() or poll_for_frames().\n"
             "The streaming loop runs until the pipeline is stopped.\n"
             "Starting the pipeline is possible only when it is not started. If the pipeline was started, an exception is raised.\n")
        .def("start", [](rs2::pipeline& self, std::function<void(rs2::frame)> f) { return self.start(f); }, "Start the pipeline streaming with its default configuration.\n"
             "The pipeline captures samples from the device, and delivers them to the through the provided frame callback.\n"
             "Starting the pipeline is possible only when it is not started. If the pipeline was started, an exception is raised.\n"
             "When starting the pipeline with a callback both wait_for_frames() and poll_for_frames() will throw exception.", "callback"_a)
        .def("start", [](rs2::pipeline& self, const rs2::config& config, std::function<void(rs2::frame)> f) { return self.start(config, f); }, "Start the pipeline streaming according to the configuraion.\n"
             "The pipeline captures samples from the device, and delivers them to the through the provided frame callback.\n"
             "Starting the pipeline is possible only when it is not started. If the pipeline was started, an exception is raised.\n"
             "When starting the pipeline with a callback both wait_for_frames() and poll_for_frames() will throw exception.\n"
             "The pipeline selects and activates the device upon start, according to configuration or a default configuration.\n"
             "When the rs2::config is provided to the method, the pipeline tries to activate the config resolve() result.\n"
             "If the application requests are conflicting with pipeline computer vision modules or no matching device is available on the platform, the method fails.\n"
             "Available configurations and devices may change between config resolve() call and pipeline start, in case devices are connected or disconnected, "
             "or another application acquires ownership of a device.", "config"_a, "callback"_a)
        .def("stop", &rs2::pipeline::stop, "Stop the pipeline streaming.\n"
             "The pipeline stops delivering samples to the attached computer vision modules and processing blocks, stops the device streaming and releases "
             "the device resources used by the pipeline. It is the application's responsibility to release any frame reference it owns.\n"
             "The method takes effect only after start() was called, otherwise an exception is raised.", py::call_guard<py::gil_scoped_release>())
        .def("wait_for_frames", &rs2::pipeline::wait_for_frames, "Wait until a new set of frames becomes available.\n"
             "The frames set includes time-synchronized frames of each enabled stream in the pipeline.\n"
             "In case of different frame rates of the streams, the frames set include a matching frame of the slow stream, which may have been included in previous frames set.\n"
             "The method blocks the calling thread, and fetches the latest unread frames set.\n"
             "Device frames, which were produced while the function wasn't called, are dropped. To avoid frame drops, this method should be called as fast as the device frame rate.\n"
             "The application can maintain the frames handles to defer processing. However, if the application maintains too long history, "
             "the device may lack memory resources to produce new frames, and the following call to this method shall fail to retrieve new "
             "frames, until resources become available.", "timeout_ms"_a = 5000, py::call_guard<py::gil_scoped_release>())
        .def("poll_for_frames", [](const rs2::pipeline &self)
        {
            rs2::frameset frames;
            self.poll_for_frames(&frames);
            return frames;
        }, "Check if a new set of frames is available and retrieve the latest undelivered set.\n"
           "The frames set includes time-synchronized frames of each enabled stream in the pipeline.\n"
           "The method returns without blocking the calling thread, with status of new frames available or not.\n"
           "If available, it fetches the latest frames set.\n"
           "Device frames, which were produced while the function wasn't called, are dropped.\n"
           "To avoid frame drops, this method should be called as fast as the device frame rate.\n"
           "The application can maintain the frames handles to defer processing. However, if the application maintains too long "
           "history, the device may lack memory resources to produce new frames, and the following calls to this method shall "
           "return no new frames, until resources become available.")
        .def("try_wait_for_frames", [](const rs2::pipeline &self, unsigned int timeout_ms)
        {
            rs2::frameset fs;
            auto success = self.try_wait_for_frames(&fs, timeout_ms);
            return std::make_tuple(success, fs);
        }, "timeout_ms"_a = 5000, py::call_guard<py::gil_scoped_release>())
        .def("get_active_profile", &rs2::pipeline::get_active_profile); // No docstring in C++

    struct pipeline_wrapper //Workaround to allow python implicit conversion of pipeline to std::shared_ptr<rs2_pipeline>
    {
        std::shared_ptr<rs2_pipeline> _ptr;
    };

    py::class_<pipeline_wrapper>(m, "pipeline_wrapper")
        .def(py::init([](rs2::pipeline p) { return pipeline_wrapper{ p }; }));

    py::implicitly_convertible<rs2::pipeline, pipeline_wrapper>();

    py::class_<rs2::pipeline_profile> pipeline_profile(m, "pipeline_profile", "The pipeline profile includes a device and a selection of active streams, with specific profiles.\n"
                                                       "The profile is a selection of the above under filters and conditions defined by the pipeline.\n"
                                                       "Streams may belong to more than one sensor of the device.");
    pipeline_profile.def(py::init<>())
        .def("get_streams", &rs2::pipeline_profile::get_streams, "Return the selected streams profiles, which are enabled in this profile.")
        .def("get_stream", &rs2::pipeline_profile::get_stream, "Return the stream profile that is enabled for the specified stream in this profile.", "stream_type"_a, "stream_index"_a = -1)
        .def("get_device", &rs2::pipeline_profile::get_device, "Retrieve the device used by the pipeline.\n"
             "The device class provides the application access to control camera additional settings - get device "
             "information, sensor options information, options value query and set, sensor specific extensions.\n"
             "Since the pipeline controls the device streams configuration, activation state and frames reading, "
             "calling the device API functions, which execute those operations, results in unexpected behavior.\n"
             "The pipeline streaming device is selected during pipeline start(). Devices of profiles, which are not returned "
             "by pipeline start() or get_active_profile(), are not guaranteed to be used by the pipeline.");


    py::class_<rs2::config> config(m, "config", "The config allows pipeline users to request filters for the pipeline streams and device selection and configuration.\n"
                                   "This is an optional step in pipeline creation, as the pipeline resolves its streaming device internally.\n"
                                   "Config provides its users a way to set the filters and test if there is no conflict with the pipeline requirements from the device.\n"
                                   "It also allows the user to find a matching device for the config filters and the pipeline, in order to select a device explicitly, "
                                   "and modify its controls before streaming starts.");
    config.def(py::init<>())
        .def("enable_stream", (void (rs2::config::*)(rs2_stream, int, int, int, rs2_format, int)) &rs2::config::enable_stream, "Enable a device stream explicitly, with selected stream parameters.\n"
             "The method allows the application to request a stream with specific configuration.\n"
             "If no stream is explicitly enabled, the pipeline configures the device and its streams according to the attached computer vision modules and processing blocks "
             "requirements, or default configuration for the first available device.\n"
             "The application can configure any of the input stream parameters according to its requirement, or set to 0 for don't care value.\n"
             "The config accumulates the application calls for enable configuration methods, until the configuration is applied.\n"
             "Multiple enable stream calls for the same stream override each other, and the last call is maintained.\n"
             "Upon calling resolve(), the config checks for conflicts between the application configuration requests and the attached computer vision "
             "modules and processing blocks requirements, and fails if conflicts are found.\n"
             "Before resolve() is called, no conflict check is done.", "stream_type"_a, "stream_index"_a, "width"_a, "height"_a, "format"_a = RS2_FORMAT_ANY, "framerate"_a = 0)
        .def("enable_stream", (void (rs2::config::*)(rs2_stream, int)) &rs2::config::enable_stream, "Stream type and possibly also stream index. Other parameters are resolved internally.", "stream_type"_a, "stream_index"_a = -1)
        .def("enable_stream", (void (rs2::config::*)(rs2_stream, rs2_format, int))&rs2::config::enable_stream, "Stream type and format, and possibly frame rate. Other parameters are resolved internally.", "stream_type"_a, "format"_a, "framerate"_a = 0)
        .def("enable_stream", (void (rs2::config::*)(rs2_stream, int, int, rs2_format, int)) &rs2::config::enable_stream, "Stream type and resolution, and possibly format and frame rate. Other parameters are resolved internally.", "stream_type"_a, "width"_a, "height"_a, "format"_a = RS2_FORMAT_ANY, "framerate"_a = 0)
        .def("enable_stream", (void (rs2::config::*)(rs2_stream, int, rs2_format, int)) &rs2::config::enable_stream, "Stream type, index, and format, and possibly framerate. Other parameters are resolved internally.", "stream_type"_a, "stream_index"_a, "format"_a, "framerate"_a = 0)
        .def("enable_all_streams", &rs2::config::enable_all_streams, "Enable all device streams explicitly.\n"
             "The conditions and behavior of this method are similar to those of enable_stream().\n"
             "This filter enables all raw streams of the selected device. The device is either selected explicitly by the application, "
             "or by the pipeline requirements or default. The list of streams is device dependent.")
        .def("enable_device", &rs2::config::enable_device, "Select a specific device explicitly by its serial number, to be used by the pipeline.\n"
             "The conditions and behavior of this method are similar to those of enable_stream().\n"
             "This method is required if the application needs to set device or sensor settings prior to pipeline streaming, "
             "to enforce the pipeline to use the configured device.", "serial"_a)
        .def("enable_device_from_file", &rs2::config::enable_device_from_file, "Select a recorded device from a file, to be used by the pipeline through playback.\n"
             "The device available streams are as recorded to the file, and resolve() considers only this device and configuration as available.\n"
             "This request cannot be used if enable_record_to_file() is called for the current config, and vice versa.", "file_name"_a, "repeat_playback"_a = true)
        .def("enable_record_to_file", &rs2::config::enable_record_to_file, "Requires that the resolved device would be recorded to file.\n"
             "This request cannot be used if enable_device_from_file() is called for the current config, and vice versa as available.", "file_name"_a)
        .def("disable_stream", &rs2::config::disable_stream, "Disable a device stream explicitly, to remove any requests on this stream profile.\n"
             "The stream can still be enabled due to pipeline computer vision module request. This call removes any filter on the stream configuration.", "stream"_a, "index"_a = -1)
        .def("disable_all_streams", &rs2::config::disable_all_streams, "Disable all device stream explicitly, to remove any requests on the streams profiles.\n"
             "The streams can still be enabled due to pipeline computer vision module request. This call removes any filter on the streams configuration.")
        .def("resolve", [](rs2::config* c, pipeline_wrapper pw) -> rs2::pipeline_profile { return c->resolve(pw._ptr); }, "Resolve the configuration filters, "
             "to find a matching device and streams profiles.\n"
             "The method resolves the user configuration filters for the device and streams, and combines them with the requirements of the computer vision modules "
             "and processing blocks attached to the pipeline. If there are no conflicts of requests, it looks for an available device, which can satisfy all requests, "
             "and selects the first matching streams configuration.\n"
             "In the absence of any request, the config object selects the first available device and the first color and depth streams configuration."
             "The pipeline profile selection during start() follows the same method. Thus, the selected profile is the same, if no change occurs to the available devices."
             "Resolving the pipeline configuration provides the application access to the pipeline selected device for advanced control."
             "The returned configuration is not applied to the device, so the application doesn't own the device sensors. However, the application can call enable_device(), "
             "to enforce the device returned by this method is selected by pipeline start(), and configure the device and sensors options or extensions before streaming starts.", "p"_a)
        .def("can_resolve", [](rs2::config* c, pipeline_wrapper pw) -> bool { return c->can_resolve(pw._ptr); }, "Check if the config can resolve the configuration filters, "
             "to find a matching device and streams profiles. The resolution conditions are as described in resolve().", "p"_a);

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
        .def("set_amp_factor", &rs400::advanced_mode::set_amp_factor, "group"_a)    //STAFactor
        .def("get_amp_factor", &rs400::advanced_mode::get_amp_factor, "mode"_a = 0) //STAFactor
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
