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

#define NAME pylibrs
#define SNAME "pylibrs"

PYBIND11_MAKE_OPAQUE(std::vector<rs::stream_profile>);


namespace py = pybind11;
using namespace pybind11::literals;

PYBIND11_PLUGIN(NAME) {
    py::module m(SNAME, "Library for accessing Intel RealSenseTM cameras");
    
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
          .value("motion_data", RS_FORMAT_MOTION_DATA)
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
                      ss << "<" SNAME ".stream_profile: "
                         << p.stream << " " << p.width
                         << "x" << p.height << " @ " << p.fps << "fps "
                         << p.format << ">";
                      return ss.str();
                  });
    
    // binds std::vector<rs::stream_profile> in an opaque manner to prevent
    // expensive copies
    py::bind_vector<std::vector<rs::stream_profile> >(m, "VectorStream_profile");
    // Implicit(?) conversion from tuple
    stream_profile.def("__init__", [](rs::stream_profile &inst, rs_stream s,
                                      int w, int h, int fps, rs_format fmt){
        new (&inst) rs::stream_profile;
        inst.stream = s;
        inst.width = w;
        inst.height = h;
        inst.fps = fps;
        inst.format = fmt;
    });
    
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
          .value("total_frame_drops", RS_OPTION_TOTAL_FRAME_DROPS)
          .value("count", RS_OPTION_COUNT);
    
    py::enum_<rs_distortion> distortion(m, "distortion");
    distortion.value("none", RS_DISTORTION_NONE)
              .value("modified_brown_conrady", RS_DISTORTION_MODIFIED_BROWN_CONRADY)
              .value("inverse_brown_conrady", RS_DISTORTION_INVERSE_BROWN_CONRADY)
              .value("ftheta", RS_DISTORTION_FTHETA)
              .value("brown_conrady", RS_DISTORTION_BROWN_CONRADY)
              .value("count", RS_DISTORTION_COUNT);
    
#define BIND_RAW_ARRAY(class, name, type, size) #name, [](const class &c) { return c.name; }

    py::class_<rs_intrinsics> intrinsics(m, "intrinsics");
    intrinsics.def_readonly("width", &rs_intrinsics::width)
              .def_readonly("height", &rs_intrinsics::height)
              .def_readonly("ppx", &rs_intrinsics::ppx)
              .def_readonly("ppy", &rs_intrinsics::ppy)
              .def_readonly("fx", &rs_intrinsics::fx)
              .def_readonly("fy", &rs_intrinsics::fy)
              .def_readonly("model", &rs_intrinsics::model)
              .def_property_readonly(BIND_RAW_ARRAY(rs_intrinsics, coeffs, float, 5));
    
    py::enum_<rs_camera_info> camera_info(m, "camera_info");
    camera_info.value("device_name", RS_CAMERA_INFO_DEVICE_NAME)
               .value("module_name", RS_CAMERA_INFO_MODULE_NAME)
               .value("device_serial_number", RS_CAMERA_INFO_DEVICE_SERIAL_NUMBER)
               .value("camera_firmware_version", RS_CAMERA_INFO_CAMERA_FIRMWARE_VERSION)
               .value("device_location", RS_CAMERA_INFO_DEVICE_LOCATION)
               .value("device_debug_op_code", RS_CAMERA_INFO_DEVICE_DEBUG_OP_CODE)
               .value("count", RS_CAMERA_INFO_COUNT);
    
    py::class_<rs_extrinsics> extrinsics(m, "extrinsics");
    extrinsics.def_property_readonly(BIND_RAW_ARRAY(rs_extrinsics, rotation, float, 9))
              .def_property_readonly(BIND_RAW_ARRAY(rs_extrinsics, translation, float, 3))
              .def("__repr__", [](const rs_extrinsics &e){
                  std::stringstream ss;
                  ss << "( (" << e.rotation[0];
                  for (int i=1; i<9; ++i) ss << ", " << e.rotation[i];
                  ss << "), (" << e.translation[0];
                  for (int i=1; i<3; ++i) ss << ", " << e.translation[i];
                  ss << ") )";
                  return ss.str();
              });
    
    py::class_<rs::option_range> option_range(m, "option_range");
    option_range.def_readwrite("min", &rs::option_range::min)
                .def_readwrite("max", &rs::option_range::max)
                .def_readwrite("default", &rs::option_range::def)
                .def_readwrite("step", &rs::option_range::step)
                .def("__repr__", [](const rs::option_range &r){
                    std::stringstream ss;
                    ss << "<" SNAME ".option_range: " << r.min << "-" << r.max
                       << "/" << r.step << " [" << r.def << "]>";
                    return ss.str();
                });
    
//    py::class_<rs::advanced> advanced(m, "advanced");
//    advanced.def(py::init<std::shared_ptr<rs_device> >())
//            .def("send_and_receive_raw_data", &rs::advanced::send_and_receive_raw_data,
//                 "input"_a);

    py::enum_<rs_timestamp_domain> ts_domain(m, "timestamp_domain");
    ts_domain.value("camera", RS_TIMESTAMP_DOMAIN_CAMERA)
             .value("external", RS_TIMESTAMP_DOMAIN_EXTERNAL)
             .value("system", RS_TIMESTAMP_DOMAIN_SYSTEM)
             .value("count", RS_TIMESTAMP_DOMAIN_COUNT);
    
    py::enum_<rs_frame_metadata> frame_metadata(m, "frame_metadata");
    frame_metadata.value("actual_exposure", RS_FRAME_METADATA_ACTUAL_EXPOSURE)
                  .value("count", RS_FRAME_METADATA_COUNT);
    
    py::class_<rs::frame> frame(m, "frame");
    frame.def("get_timestamp", &rs::frame::get_timestamp, "Retrieve the time "
              "at which the frame was captured in milliseconds")
         .def("get_frame_timestamp_domain", &rs::frame::get_frame_timestamp_domain,
              "Retrieve the timestamp domain (clock name)")
         .def("get_frame_metadata", &rs::frame::get_frame_metadata, "Retrieve "
              "the current value of a single frame_metadata",
              "frame_metadata"_a)
         .def("supports_frame_metadata", &rs::frame::supports_frame_metadata,
              "Determine if the device allows a specific metadata to be "
              "queried", "frame_metadata"_a)
         .def("get_frame_number", &rs::frame::get_frame_number, "Retrieve "
              "frame number (from frame handle)")
         .def("get_data", [](const rs::frame& f) /*-> py::list*/ {
                auto *data = static_cast<const uint8_t*>(f.get_data());
                long size = f.get_width()*f.get_height()*f.get_bytes_per_pixel();
                return /*py::cast*/(std::vector<uint8_t>(data, data + size));
            }, "retrieve data from frame handle")
         .def("get_width", &rs::frame::get_width, "Returns image width in "
              "pixels")
         .def("get_height", &rs::frame::get_height, "Returns image height in "
              "pixels")
         .def("get_stride_in_bytes", &rs::frame::get_stride_in_bytes,
              "Retrieve frame stride, meaning the actual line width in memory "
              "in bytes (not the logical image width)")
         .def("get_bits_per_pixel", &rs::frame::get_bits_per_pixel, "Retrieve "
              "bits per pixel")
         .def("get_bytes_per_pixel", &rs::frame::get_bytes_per_pixel)
         .def("get_format", &rs::frame::get_format, "Retrieve pixel format of "
              "the frame")
         .def("get_stream_type", &rs::frame::get_stream_type, "Retrieve the "
              "origin stream type that produced the frame");
//       .def("try_clone_ref", &rs::frame::try_clone_ref);

    py::class_<rs::device_list> device_list(m, "device_list");
    device_list.def("__getitem__", [](const rs::device_list& d, size_t i){
                       if (i >= d.size())
                           throw py::index_error();
                       return d[i];
                })
               .def("__len__", &rs::device_list::size)
               .def("size", &rs::device_list::size)
               .def("__iter__", [](const rs::device_list &d){
                       return py::make_iterator(d.begin(), d.end());
                   }, py::keep_alive<0, 1>())
               .def("__getitem__", [](const rs::device_list &d, py::slice slice){
                       size_t start, stop, step, slicelength;
                       if (!slice.compute(d.size(), &start, &stop, &step, &slicelength))
                           throw py::error_already_set();
                       auto *dlist = new std::vector<rs::device>(slicelength);
                       for (size_t i = 0; i < slicelength; ++i) {
                           (*dlist)[i] = d[start];
                           start += step;
                       }
                       return dlist;
                   })
               .def("front", &rs::device_list::front)
               .def("back", &rs::device_list::back);

    
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
          .def(py::self == py::self)
          .def(py::self != py::self)
          .def("__repr__", [](const rs::device &dev){
              std::stringstream ss;
              ss << "<" SNAME ".device: " << dev.get_camera_info(RS_CAMERA_INFO_DEVICE_NAME)
                 << " (S/N: " << dev.get_camera_info(RS_CAMERA_INFO_DEVICE_SERIAL_NUMBER)
                 << ") " << dev.get_camera_info(RS_CAMERA_INFO_MODULE_NAME)
                 << " module>";
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
