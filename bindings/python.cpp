#include <pybind11/pybind11.h>

// convenience functions
#include <pybind11/operators.h>


// STL conversions
#include <pybind11/stl.h>

// makes certain STL containers opaque to prevent expensive copies
#include <pybind11/stl_bind.h>

// makes std::function conversions work
#include <pybind11/functional.h>

#include "../include/librealsense/rs.h"
#include "../include/librealsense/rs.hpp"

PYBIND11_MAKE_OPAQUE(std::vector<rs::stream_profile>);


namespace py = pybind11;
using namespace pybind11::literals;

PYBIND11_PLUGIN(pylibrs) {
    py::module m("pylibrs", "Library for accessing Intel RealSenseTM cameras");
    
    py::enum_<rs_stream> stream(m, "stream");
    stream.value("any", RS_STREAM_ANY)
          .value("depth", RS_STREAM_DEPTH)
          .value("color", RS_STREAM_COLOR)
          .value("infrared", RS_STREAM_INFRARED)
          .value("infrared2", RS_STREAM_INFRARED2)
          .value("fisheye", RS_STREAM_FISHEYE)
          .value("gyro", RS_STREAM_GYRO)
          .value("accel", RS_STREAM_ACCEL)
          .value("count", RS_STREAM_COUNT);
    
    py::enum_<rs_format> format(m, "format");
    format.value("any", RS_FORMAT_ANY)
          .value("z16", RS_FORMAT_Z16)
          .value("disparity16", RS_FORMAT_DISPARITY16)
          .value("xyz32f", RS_FORMAT_XYZ32F)
          .value("yuyv", RS_FORMAT_YUYV)
          .value("rgb8", RS_FORMAT_RGB8)
          .value("bgr8", RS_FORMAT_BGR8)
          .value("rgba8", RS_FORMAT_RGBA8)
          .value("bgra8", RS_FORMAT_BGRA8)
          .value("y8", RS_FORMAT_Y8)
          .value("y16", RS_FORMAT_Y16)
          .value("raw10", RS_FORMAT_RAW10)
          .value("raw16", RS_FORMAT_RAW16)
          .value("raw8", RS_FORMAT_RAW8)
          .value("uyvy", RS_FORMAT_UYVY)
          .value("count", RS_FORMAT_COUNT);
    
    py::class_<rs::stream_profile> stream_profile(m, "stream_profile");
    stream_profile.def("match", &rs::stream_profile::match)
                  .def("contradicts", &rs::stream_profile::contradicts)
                  .def("has_wildcards", &rs::stream_profile::has_wildcards)
                  .def(py::self == py::self)
                  .def(py::init<>())
//                  .def(py::init<rs_stream, int, int, int, rs_format>())
                  .def_readwrite("stream", &rs::stream_profile::stream)
                  .def_readwrite("width", &rs::stream_profile::width)
                  .def_readwrite("height", &rs::stream_profile::height)
                  .def_readwrite("fps", &rs::stream_profile::fps)
                  .def_readwrite("format", &rs::stream_profile::format)
                  .def("__repr__", [](const rs::stream_profile &p){
                      std::stringstream ss;
                      ss << rs_stream_to_string(p.stream) << " " << p.width
                         << "x" << p.height << " @ " << p.fps << "fps "
                         << rs_format_to_string(p.format);
                      return ss.str();
                  });
    
    // binds std::vector<rs::stream_profile> in an opaque manner to prevent
    // expensive copies
    py::bind_vector<std::vector<rs::stream_profile> >(m, "VectorStream_profile");
    
    py::enum_<rs_option> option(m, "option");
    option.value("backlight_compensation", RS_OPTION_BACKLIGHT_COMPENSATION)
          .value("brightness", RS_OPTION_BRIGHTNESS)
          .value("contrast", RS_OPTION_CONTRAST)
          .value("exposure", RS_OPTION_EXPOSURE)
          .value("gain", RS_OPTION_GAIN)
          .value("gamma", RS_OPTION_GAMMA)
          .value("hue", RS_OPTION_HUE)
          .value("saturation", RS_OPTION_SATURATION)
          .value("sharpness", RS_OPTION_SHARPNESS)
          .value("white_balance", RS_OPTION_WHITE_BALANCE)
          .value("enable_auto_exposure", RS_OPTION_ENABLE_AUTO_EXPOSURE)
          .value("enable_auto_white_balance", RS_OPTION_ENABLE_AUTO_WHITE_BALANCE)
          .value("visual_preset", RS_OPTION_VISUAL_PRESET)
          .value("laser_power", RS_OPTION_LASER_POWER)
          .value("accuracy", RS_OPTION_ACCURACY)
          .value("motion_range", RS_OPTION_MOTION_RANGE)
          .value("filter_option", RS_OPTION_FILTER_OPTION)
          .value("confidence_threshold", RS_OPTION_CONFIDENCE_THRESHOLD)
          .value("emitter_enabled", RS_OPTION_EMITTER_ENABLED)
          .value("frames_queue_size", RS_OPTION_FRAMES_QUEUE_SIZE)
          .value("hardware_logger_enabled", RS_OPTION_HARDWARE_LOGGER_ENABLED)
          .value("total_frame_drops", RS_OPTION_TOTAL_FRAME_DROPS)
          .value("count", RS_OPTION_COUNT);
    
    py::enum_<rs_distortion> distortion(m, "distortion");
    distortion.value("none", RS_DISTORTION_NONE)
              .value("modified_brown_conrady", RS_DISTORTION_MODIFIED_BROWN_CONRADY)
              .value("inverse_brown_conrady", RS_DISTORTION_INVERSE_BROWN_CONRADY)
              .value("ftheta", RS_DISTORTION_FTHETA)
              .value("brown_conrady", RS_DISTORTION_BROWN_CONRADY)
              .value("count", RS_DISTORTION_COUNT);
    
#define BIND_RAW_ARRAY(class, name, type, size) #name, [](class &c, std::array<type, size> a){ for (int i=0; i<size; ++i) c.name[i] = a[i]; }, [](const class &c) { return c.name; }

    py::class_<rs_intrinsics> intrinsics(m, "intrinsics");
    intrinsics.def_readwrite("width", &rs_intrinsics::width)
              .def_readwrite("height", &rs_intrinsics::height)
              .def_readwrite("ppx", &rs_intrinsics::ppx)
              .def_readwrite("ppy", &rs_intrinsics::ppy)
              .def_readwrite("fx", &rs_intrinsics::fx)
              .def_readwrite("fy", &rs_intrinsics::fy)
              .def_readwrite("model", &rs_intrinsics::model)
              .def_property(BIND_RAW_ARRAY(rs_intrinsics, coeffs, float, 5));
    
    py::enum_<rs_camera_info> camera_info(m, "camera_info");
    camera_info.value("device_name", RS_CAMERA_INFO_DEVICE_NAME)
               .value("module_name", RS_CAMERA_INFO_MODULE_NAME)
               .value("device_serial_number", RS_CAMERA_INFO_DEVICE_SERIAL_NUMBER)
               .value("camera_firmware_version", RS_CAMERA_INFO_CAMERA_FIRMWARE_VERSION)
               .value("device_location", RS_CAMERA_INFO_DEVICE_LOCATION)
               .value("count", RS_CAMERA_INFO_COUNT);
    
    py::class_<rs_extrinsics> extrinsics(m, "extrinsics");
    extrinsics.def_property(BIND_RAW_ARRAY(rs_extrinsics, rotation, float, 9))
              .def_property(BIND_RAW_ARRAY(rs_extrinsics, translation, float, 3));
    
//    py::class_<rs::advanced> advanced(m, "advanced");
//    advanced.def(py::init<std::shared_ptr<rs_device> >())
//            .def("send_and_receive_raw_data", &rs::advanced::send_and_receive_raw_data,
//                 "input"_a);
    
    // in general frame seems to be broken because of the deleted constructor
    py::class_<rs::frame> frame(m, "frame");
//    frame.def(py::init<>())
//    //   .def(py::init<rs_frame*>()) // user shouldnt ever need this constructor
//         .def(py::init<rs::frame &&>())
//    /*     .def(py::self = py::self) */ // broken
//         .def("swap", &rs::frame::swap, "other"_a)
//    /*   .def( <bool conversion> ) */;
    
    // wrap rs::device
    py::class_<rs::device> device(m, "device");
    device.def("open", (void (rs::device::*)(const rs::stream_profile&) const) &rs::device::open,
               "Open subdevice for exclusive access, by commiting to a "
               "configuration.", "profile"_a)
          .def("open", (void (rs::device::*)(const std::vector<rs::stream_profile>&) const) &rs::device::open,
               "Open subdevice for exclusive access, by committing to a "
               "composite configuration, specifying one or more stream "
               "profiles.\nThis method should be used for interdependant "
               "streams, such as depth and infrared, that must be configured "
               "together.", "profiles"_a)
          .def("close", &rs::device::close, "Close subdevice for exclusive "
               "access.\nThis method should be used for releasing device "
               "resources.")
          .def("start", [](const rs::device& dev, std::function<void(rs::frame)> callback){
              dev.start(callback);
          }, "Start streaming.", "callback"_a)
          .def("stop", &rs::device::stop, "Stop streaming.")
          .def("get_option", &rs::device::get_option, "Read option value from "
               "the device.", "option"_a)
          .def("get_option_range", &rs::device::get_option_range, "Retrieve "
               "the available range of values of a supported option.",
               "option"_a)
          .def("set_option", &rs::device::set_option, "Write new value to "
               "device option.", "option"_a, "value"_a)
          .def("supports", (bool (rs::device::*)(rs_option) const) &rs::device::supports,
               "Check if a particular option is supported by a subdevice.",
               "option"_a)
          .def("get_option_description", &rs::device::get_option_description,
               "Get option description.", "option"_a)
          .def("get_option_value_description", &rs::device::get_option_value_description,
               "Get option value description (in case a specific option value "
               "holds special meaning.)", "option"_a, "val"_a)
          .def("get_stream_modes", &rs::device::get_stream_modes, "Check if "
               "physical subdevice is supported.")
          .def("get_intrinsics", &rs::device::get_intrinsics, "Retrieve stream"
               " intrinsics", "profile"_a)
          .def("supports", (bool (rs::device::*)(rs_camera_info) const) &rs::device::supports,
               "Check if specific camera info is supported.", "info"_a)
          .def("get_camera_info", &rs::device::get_camera_info, "Retrieve "
               "camera specific information, like versions of various internal"
               " components.", "info"_a)
          .def("get_adjacent_devices", &rs::device::get_adjacent_devices,
               "Returns the list of adjacent devices, sharing the same "
               "physical parent composite device.")
          .def("get_extrinsics_to", &rs::device::get_extrinsics_to,
               "to_device"_a)
          .def("get_depth_scale", &rs::device::get_depth_scale, "Retrieve "
               "mapping between the units of the depth image and meters.")
//          .def("debug", &rs::device::debug)
          .def(py::init<>())
          .def(py::self == py::self)
          .def(py::self != py::self)
          .def("__repr__", [](const rs::device &dev){
              std::stringstream ss;
              ss << "librs " << dev.get_camera_info(RS_CAMERA_INFO_DEVICE_NAME)
                 << " (S/N: " << dev.get_camera_info(RS_CAMERA_INFO_DEVICE_SERIAL_NUMBER)
                 << ")" << dev.get_camera_info(RS_CAMERA_INFO_MODULE_NAME) << " module";
              return ss.str();
          });

    // Bring the context class into python
    py::class_<rs::context> context(m, "context");
    context.def(py::init<>())
           .def("query_devices", &rs::context::query_devices, "Create a static"
                " snapshot of all connected devices a the time of the call")
           .def("get_extrinsics", &rs::context::get_extrinsics,
                "from_device"_a, "to_device"_a);
    return m.ptr();
}
