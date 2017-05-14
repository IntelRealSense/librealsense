#include <pybind11/pybind11.h>

// convenience functions
#include <pybind11/operators.h>

// STL conversions
#include <pybind11/stl.h>

// makes certain STL containers opaque to prevent expensive copies
#include <pybind11/stl_bind.h>

// makes std::function conversions work
#include <pybind11/functional.h>

#include "../include/librealsense/rs2.h"
#include "../include/librealsense/rs2.hpp"

#define NAME pyrealsense2
#define SNAME "pyrealsense2"

PYBIND11_MAKE_OPAQUE(std::vector<rs2::stream_profile>)


namespace py = pybind11;
using namespace pybind11::literals;

PYBIND11_PLUGIN(NAME) {
    py::module m(SNAME, "Library for accessing Intel RealSenseTM cameras");
    
    py::enum_<rs2_stream> stream(m, "stream");
    stream.value("any", RS2_STREAM_ANY)
        .value("depth", RS2_STREAM_DEPTH)
        .value("color", RS2_STREAM_COLOR)
        .value("infrared", RS2_STREAM_INFRARED)
        .value("infrared2", RS2_STREAM_INFRARED2)
        .value("fisheye", RS2_STREAM_FISHEYE)
        .value("gyro", RS2_STREAM_GYRO)
        .value("accel", RS2_STREAM_ACCEL)
        .value("count", RS2_STREAM_COUNT);
    
    py::enum_<rs2_format> format(m, "format");
    format.value("any", RS2_FORMAT_ANY)
          .value("z16", RS2_FORMAT_Z16)
          .value("disparity16", RS2_FORMAT_DISPARITY16)
          .value("xyz32f", RS2_FORMAT_XYZ32F)
          .value("yuyv", RS2_FORMAT_YUYV)
          .value("rgb8", RS2_FORMAT_RGB8)
          .value("bgr8", RS2_FORMAT_BGR8)
          .value("rgba8", RS2_FORMAT_RGBA8)
          .value("bgra8", RS2_FORMAT_BGRA8)
          .value("y8", RS2_FORMAT_Y8)
          .value("y16", RS2_FORMAT_Y16)
          .value("raw10", RS2_FORMAT_RAW10)
          .value("raw16", RS2_FORMAT_RAW16)
          .value("raw8", RS2_FORMAT_RAW8)
          .value("uyvy", RS2_FORMAT_UYVY)
          .value("motion_raw", RS2_FORMAT_MOTION_RAW)
          .value("motion_xyz32f", RS2_FORMAT_MOTION_XYZ32F)
          .value("count", RS2_FORMAT_COUNT);
    
    py::class_<rs2::stream_profile> stream_profile(m, "stream_profile");
    stream_profile.def("match", &rs2::stream_profile::match)
                  .def("contradicts", &rs2::stream_profile::contradicts)
                  .def("has_wildcards", &rs2::stream_profile::has_wildcards)
                  .def(py::self == py::self)
                  .def(py::init<>())
//                  .def(py::init<rs2_stream, int, int, int, rs2_format>())
                  .def_readwrite("stream", &rs2::stream_profile::stream)
                  .def_readwrite("width", &rs2::stream_profile::width)
                  .def_readwrite("height", &rs2::stream_profile::height)
                  .def_readwrite("fps", &rs2::stream_profile::fps)
                  .def_readwrite("format", &rs2::stream_profile::format)
                  .def("__repr__", [](const rs2::stream_profile &p){
                      std::stringstream ss;
                      ss << "<" SNAME ".stream_profile: "
                         << p.stream << " " << p.width
                         << "x" << p.height << " @ " << p.fps << "fps "
                         << p.format << ">";
                      return ss.str();
                  });
    
    // binds std::vector<rs2::stream_profile> in an opaque manner to prevent
    // expensive copies
    py::bind_vector<std::vector<rs2::stream_profile> >(m, "VectorStream_profile");
    // Implicit(?) conversion from tuple
    stream_profile.def("__init__", [](rs2::stream_profile &inst, rs2_stream s,
                                      int w, int h, int fps, rs2_format fmt){
        new (&inst) rs2::stream_profile;
        inst.stream = s;
        inst.width = w;
        inst.height = h;
        inst.fps = fps;
        inst.format = fmt;
    });

    py::enum_<rs2_notification_category> notification_category(m, "notification_category");
    notification_category.value("frames_timeout", rs2_notification_category::RS2_NOTIFICATION_CATEGORY_FRAMES_TIMEOUT)
                         .value("frame_corrupted", rs2_notification_category::RS2_NOTIFICATION_CATEGORY_FRAME_CORRUPTED)
                         .value("hardware_error", rs2_notification_category::RS2_NOTIFICATION_CATEGORY_HARDWARE_ERROR)
                         .value("unknown_error", rs2_notification_category::RS2_NOTIFICATION_CATEGORY_UNKNOWN_ERROR)
                         .value("count", rs2_notification_category::RS2_NOTIFICATION_CATEGORY_COUNT);

    py::class_<rs2::notification> notification(m, "notification");
    notification.def(py::init<>())
                .def("get_description", &rs2::notification::get_description,
                     "Retrieve the notification's description.")
                .def("get_timestamp", &rs2::notification::get_timestamp,
                     "Retrieve the notification's arrival timestamp.")
                .def("get_severity", &rs2::notification::get_severity,
                     "Retrieve the notification's severity.")
                .def("get_category", &rs2::notification::get_category,
                     "Retrieve the notification's category.");

    py::enum_<rs2_option> option(m, "option");
    option.value("backlight_compensation", RS2_OPTION_BACKLIGHT_COMPENSATION)
          .value("brightness", RS2_OPTION_BRIGHTNESS)
          .value("contrast", RS2_OPTION_CONTRAST)
          .value("exposure", RS2_OPTION_EXPOSURE)
          .value("gain", RS2_OPTION_GAIN)
          .value("gamma", RS2_OPTION_GAMMA)
          .value("hue", RS2_OPTION_HUE)
          .value("saturation", RS2_OPTION_SATURATION)
          .value("sharpness", RS2_OPTION_SHARPNESS)
          .value("white_balance", RS2_OPTION_WHITE_BALANCE)
          .value("enable_auto_exposure", RS2_OPTION_ENABLE_AUTO_EXPOSURE)
          .value("enable_auto_white_balance", RS2_OPTION_ENABLE_AUTO_WHITE_BALANCE)
          .value("visual_preset", RS2_OPTION_VISUAL_PRESET)
          .value("laser_power", RS2_OPTION_LASER_POWER)
          .value("accuracy", RS2_OPTION_ACCURACY)
          .value("motion_range", RS2_OPTION_MOTION_RANGE)
          .value("filter_option", RS2_OPTION_FILTER_OPTION)
          .value("confidence_threshold", RS2_OPTION_CONFIDENCE_THRESHOLD)
          .value("emitter_enabled", RS2_OPTION_EMITTER_ENABLED)
          .value("frames_queue_size", RS2_OPTION_FRAMES_QUEUE_SIZE)
          .value("total_frame_drops", RS2_OPTION_TOTAL_FRAME_DROPS)
          .value("count", RS2_OPTION_COUNT);
    
    py::enum_<rs2_distortion> distortion(m, "distortion");
    distortion.value("none", RS2_DISTORTION_NONE)
              .value("modified_brown_conrady", RS2_DISTORTION_MODIFIED_BROWN_CONRADY)
              .value("inverse_brown_conrady", RS2_DISTORTION_INVERSE_BROWN_CONRADY)
              .value("ftheta", RS2_DISTORTION_FTHETA)
              .value("brown_conrady", RS2_DISTORTION_BROWN_CONRADY)
              .value("count", RS2_DISTORTION_COUNT);
    
#define BIND_RAW_ARRAY(class, name, type, size) #name, [](const class &c) -> const std::array<type, size>& { return reinterpret_cast<const std::array<type, size>&>(c.name); }

    py::class_<rs2_intrinsics> intrinsics(m, "intrinsics");
    intrinsics.def_readonly("width", &rs2_intrinsics::width)
              .def_readonly("height", &rs2_intrinsics::height)
              .def_readonly("ppx", &rs2_intrinsics::ppx)
              .def_readonly("ppy", &rs2_intrinsics::ppy)
              .def_readonly("fx", &rs2_intrinsics::fx)
              .def_readonly("fy", &rs2_intrinsics::fy)
              .def_readonly("model", &rs2_intrinsics::model)
              .def_property_readonly(BIND_RAW_ARRAY(rs2_intrinsics, coeffs, float, 5));
    
    py::enum_<rs2_camera_info> camera_info(m, "camera_info");
    camera_info.value("device_name", RS2_CAMERA_INFO_DEVICE_NAME)
               .value("module_name", RS2_CAMERA_INFO_MODULE_NAME)
               .value("device_serial_number", RS2_CAMERA_INFO_DEVICE_SERIAL_NUMBER)
               .value("camera_firmware_version", RS2_CAMERA_INFO_CAMERA_FIRMWARE_VERSION)
               .value("device_location", RS2_CAMERA_INFO_DEVICE_LOCATION)
               .value("device_debug_op_code", RS2_CAMERA_INFO_DEVICE_DEBUG_OP_CODE)
               .value("advanced_mode", RS2_CAMERA_INFO_ADVANCED_MODE)
               .value("count", RS2_CAMERA_INFO_COUNT);
    
    py::class_<rs2_extrinsics> extrinsics(m, "extrinsics");
    extrinsics.def_property_readonly(BIND_RAW_ARRAY(rs2_extrinsics, rotation, float, 9))
              .def_property_readonly(BIND_RAW_ARRAY(rs2_extrinsics, translation, float, 3))
              .def("__repr__", [](const rs2_extrinsics &e){
                  std::stringstream ss;
                  ss << "( (" << e.rotation[0];
                  for (int i=1; i<9; ++i) ss << ", " << e.rotation[i];
                  ss << "), (" << e.translation[0];
                  for (int i=1; i<3; ++i) ss << ", " << e.translation[i];
                  ss << ") )";
                  return ss.str();
              });

    py::class_<rs2::region_of_interest> region_of_interest(m, "region_of_interest");
    region_of_interest.def_readwrite("min_x", &rs2::region_of_interest::min_x)
                      .def_readwrite("min_y", &rs2::region_of_interest::min_y)
                      .def_readwrite("max_x", &rs2::region_of_interest::max_x)
                      .def_readwrite("max_y", &rs2::region_of_interest::max_y);

    py::class_<rs2::option_range> option_range(m, "option_range");
    option_range.def_readwrite("min", &rs2::option_range::min)
                .def_readwrite("max", &rs2::option_range::max)
                .def_readwrite("default", &rs2::option_range::def)
                .def_readwrite("step", &rs2::option_range::step)
                .def("__repr__", [](const rs2::option_range &r){
                    std::stringstream ss;
                    ss << "<" SNAME ".option_range: " << r.min << "-" << r.max
                       << "/" << r.step << " [" << r.def << "]>";
                    return ss.str();
                });
    
//    py::class_<rs2::advanced> advanced(m, "advanced");
//    advanced.def(py::init<std::shared_ptr<rs2_device> >())
//            .def("send_and_receive_raw_data", &rs2::advanced::send_and_receive_raw_data,
//                 "input"_a);

    py::enum_<rs2_timestamp_domain> ts_domain(m, "timestamp_domain");
    ts_domain.value("hardware_clock", RS2_TIMESTAMP_DOMAIN_HARDWARE_CLOCK)
             .value("system_time", RS2_TIMESTAMP_DOMAIN_SYSTEM_TIME)
             .value("count", RS2_TIMESTAMP_DOMAIN_COUNT);
    
    py::enum_<rs2_frame_metadata> frame_metadata(m, "frame_metadata");
    frame_metadata.value("actual_exposure", RS2_FRAME_METADATA_ACTUAL_EXPOSURE)
                  .value("count", RS2_FRAME_METADATA_COUNT);
    
    py::class_<rs2::frame> frame(m, "frame");
    frame.def("get_timestamp", &rs2::frame::get_timestamp, "Retrieve the time "
              "at which the frame was captured in milliseconds")
         .def("get_frame_timestamp_domain", &rs2::frame::get_frame_timestamp_domain,
              "Retrieve the timestamp domain (clock name)")
         .def("get_frame_metadata", &rs2::frame::get_frame_metadata, "Retrieve "
              "the current value of a single frame_metadata",
              "frame_metadata"_a)
         .def("supports_frame_metadata", &rs2::frame::supports_frame_metadata,
              "Determine if the device allows a specific metadata to be "
              "queried", "frame_metadata"_a)
         .def("get_frame_number", &rs2::frame::get_frame_number,
              /*py::return_value_policy::copy, */"Retrieve frame number (from frame handle)")
         .def("get_data", [](const rs2::frame& f) /*-> py::list*/ {
                auto *data = static_cast<const uint8_t*>(f.get_data());
                long size = f.get_width()*f.get_height()*f.get_bytes_per_pixel();
                return /*py::cast*/(std::vector<uint8_t>(data, data + size));
            }, /*py::return_value_policy::take_ownership, */"retrieve data from frame handle")
         .def("get_width", &rs2::frame::get_width, "Returns image width in "
              "pixels")
         .def("get_height", &rs2::frame::get_height, "Returns image height in "
              "pixels")
         .def("get_stride_in_bytes", &rs2::frame::get_stride_in_bytes,
              "Retrieve frame stride, meaning the actual line width in memory "
              "in bytes (not the logical image width)")
         .def("get_bits_per_pixel", &rs2::frame::get_bits_per_pixel, "Retrieve "
              "bits per pixel")
         .def("get_bytes_per_pixel", &rs2::frame::get_bytes_per_pixel)
         .def("get_format", &rs2::frame::get_format, "Retrieve pixel format of "
              "the frame")
         .def("get_stream_type", &rs2::frame::get_stream_type, "Retrieve the "
              "origin stream type that produced the frame");
//       .def("try_clone_ref", &rs2::frame::try_clone_ref);

    py::class_<rs2::device_list> device_list(m, "device_list");
    device_list.def("__getitem__", [](const rs2::device_list& d, size_t i){
                       if (i >= d.size())
                           throw py::index_error();
                       return d[i];
                })
               .def("__len__", &rs2::device_list::size)
               .def("size", &rs2::device_list::size)
               .def("__iter__", [](const rs2::device_list &d){
                       return py::make_iterator(d.begin(), d.end());
                   }, py::keep_alive<0, 1>())
               .def("__getitem__", [](const rs2::device_list &d, py::slice slice){
                       size_t start, stop, step, slicelength;
                       if (!slice.compute(d.size(), &start, &stop, &step, &slicelength))
                           throw py::error_already_set();
                       auto *dlist = new std::vector<rs2::device>(slicelength);
                       for (size_t i = 0; i < slicelength; ++i) {
                           (*dlist)[i] = d[start];
                           start += step;
                       }
                       return dlist;
                   })
               .def("front", &rs2::device_list::front)
               .def("back", &rs2::device_list::back);

    
    py::class_<rs2::frame_queue> frame_queue(m, "frame_queue");
    frame_queue.def(py::init<unsigned int>())
               .def(py::init<>())
               .def("wait_for_frame", &rs2::frame_queue::wait_for_frame)
               // TODO: find more pythonic way to expose this function
               .def("poll_for_frame", &rs2::frame_queue::poll_for_frame, "f"_a);

    py::class_<rs2::syncer> syncer(m, "syncer");
    syncer.def("wait_for_frames", &rs2::syncer::wait_for_frames, "timeout_ms"_a=5000,
               "Wait until a coherent set of frames becomes available.")
          .def("poll_for_frames", [](const rs2::syncer &sync)
                {
                    rs2::frameset frames;
                    sync.poll_for_frames(&frames);
                    return frames;
                }, "Check if a coherent set of frames is available");

   /* py::class_<rs2_motion_device_intrinsic> motion_device_inrinsic(m, "motion_device_intrinsic");
    motion_device_inrinsic.def_property_readonly(BIND_RAW_ARRAY(rs2_motion_device_intrinsic, noise_variances, float, 3))
                          .def_property_readonly(BIND_RAW_ARRAY(rs2_motion_device_intrinsic, bias_variances, float, 3));*/

    // wrap rs2::device
    py::class_<rs2::device> device(m, "device");
    device.def("open", (void (rs2::device::*)(const rs2::stream_profile&) const) &rs2::device::open,
               "Open subdevice for exclusive access, by commiting to a "
               "configuration.", "profile"_a)
          .def("open", (void (rs2::device::*)(const std::vector<rs2::stream_profile>&) const) &rs2::device::open,
               "Open subdevice for exclusive access, by committing to a "
               "composite configuration, specifying one or more stream "
               "profiles.\nThis method should be used for interdependant "
               "streams, such as depth and infrared, that must be configured "
               "together.", "profiles"_a)
          .def("close", [](const rs2::device& dev){
              // releases the python GIL while close is running to prevent callback deadlock
              py::gil_scoped_release release;
              dev.close();
          }, "Close subdevice for exclusive access.\nThis method should be used "
             "for releasing device resources.")
          .def("start", [](const rs2::device& dev, std::function<void(rs2::frame)> callback){
              dev.start(callback);
          }, "Start passing frames into user provided callback.", "callback"_a)
          .def("start", [](const rs2::device& dev, rs2::syncer syncer){
              dev.start(syncer);
          }, "Start passing frames into user provided callback.", "syncer"_a)
          .def("start", [](const rs2::device& dev, rs2::frame_queue& queue) {
              dev.start(queue);
          }, "Start passing frames into user provided callback.", "queue"_a)
          .def("start", [](const rs2::device& dev, rs2_stream s, std::function<void(rs2::frame)> callback) {
              dev.start(s, callback);
          }, "Start passing frames of a specific stream into user provided callback.", "stream"_a, "syncer"_a)
          .def("start", [](const rs2::device& dev, rs2_stream s, rs2::syncer syncer) {
              dev.start(s, syncer);
          }, "Start passing frames of a specific stream into user provided callback.", "stream"_a, "queue"_a)
          .def("start", [](const rs2::device& dev, rs2_stream s, rs2::frame_queue& queue) {
              dev.start(s, queue);
          }, "Start passing frames of a specific stream into user provided callback.", "stream"_a, "queue"_a)
          .def("stop", (void (rs2::device::*)(void) const) &rs2::device::stop, "Stop streaming.")
          .def("stop", (void (rs2::device::*)(rs2_stream) const) &rs2::device::stop, "Stop streaming of a specific stream.")
          .def("is_option_read_only", &rs2::device::is_option_read_only, "Check if a"
               "particular option is read only.", "option"_a)
          .def("set_notifications_callback", [](const rs2::device& dev, std::function<void(rs2::notification)> callback){
              dev.set_notifications_callback(callback);
          }, "Register notifications callback", "callback"_a)
          .def("get_option", &rs2::device::get_option, "Read option value from "
               "the device.", "option"_a)
          .def("get_option_range", &rs2::device::get_option_range, "Retrieve "
               "the available range of values of a supported option.",
               "option"_a)
          .def("set_option", &rs2::device::set_option, "Write new value to "
               "device option.", "option"_a, "value"_a)
          .def("supports", (bool (rs2::device::*)(rs2_option) const) &rs2::device::supports,
               "Check if a particular option is supported by a subdevice.",
               "option"_a)
          .def("set_region_of_interest", &rs2::device::set_region_of_interest,
               "Sets the region of interest of the auto-exposure algorithm", "roi"_a)
          .def("get_region_of_interest", &rs2::device::get_region_of_interest,
               "Gets the current region of interest of the auto-exposure algorithm")
          .def("get_option_description", &rs2::device::get_option_description,
               "Get option description.", "option"_a)
          .def("get_option_value_description", &rs2::device::get_option_value_description,
               "Get option value description (in case a specific option value "
               "holds special meaning.)", "option"_a, "val"_a)
          .def("get_stream_modes", &rs2::device::get_stream_modes, "Check if "
               "physical subdevice is supported.")
          .def("get_intrinsics", &rs2::device::get_intrinsics, "Retrieve stream"
               " intrinsics", "profile"_a)
          .def("supports", (bool (rs2::device::*)(rs2_camera_info) const) &rs2::device::supports,
               "Check if specific camera info is supported.", "info"_a)
          .def("get_camera_info", &rs2::device::get_camera_info, "Retrieve "
               "camera specific information, like versions of various internal"
               " components.", "info"_a)
          .def("get_adjacent_devices", &rs2::device::get_adjacent_devices,
               "Returns the list of adjacent devices, sharing the same "
               "physical parent composite device.")
          .def("create_syncer", &rs2::device::create_syncer, "Create frames syncronization"
               " primitive suitable for this device")
          .def("get_extrinsics_to", &rs2::device::get_extrinsics_to,
               "from_stream"_a, "to_device"_a, "to_stream"_a)
          /*.def("get_motion_intrinsics", &rs2::device::get_motion_intrinsics, "stream"_a,
               "Returns scale and bias of a motion stream")*/
          .def("hardware_reset", &rs2::device::hardware_reset, "Send hardware reset request to the device")
//          .def("debug", &rs2::device::debug)
          .def(py::self == py::self)
          .def(py::self != py::self)
          .def("__repr__", [](const rs2::device &dev){
              std::stringstream ss;
              ss << "<" SNAME ".device: " << dev.get_camera_info(RS2_CAMERA_INFO_DEVICE_NAME)
                 << " (S/N: " << dev.get_camera_info(RS2_CAMERA_INFO_DEVICE_SERIAL_NUMBER)
                 << ") " << dev.get_camera_info(RS2_CAMERA_INFO_MODULE_NAME)
                 << " module>";
              return ss.str();
          });

    // Bring the context class into python
    py::class_<rs2::context> context(m, "context");
    context.def(py::init<>())
        .def("query_devices", &rs2::context::query_devices, "Create a static"
            " snapshot of all connected devices a the time of the call")
        .def("get_extrinsics", [](const rs2::context& ctx, const rs2::device& from, rs2_stream from_stream,
                                                           const rs2::device& to, rs2_stream to_stream)
    {
        return ctx.get_extrinsics(from, from_stream, to, to_stream);
    }, "Get extrinsics",
            "from_device"_a, "from_stream"_a, "to_device"_a, "to_stream"_a)
        .def("get_time", &rs2::context::get_time);

    py::enum_<rs2_log_severity> log_severity(m, "log_severity");
    log_severity.value("debug", RS2_LOG_SEVERITY_DEBUG)
                .value("info", RS2_LOG_SEVERITY_INFO)
                .value("warn", RS2_LOG_SEVERITY_WARN)
                .value("error", RS2_LOG_SEVERITY_ERROR)
                .value("fatal", RS2_LOG_SEVERITY_FATAL)
                .value("none", RS2_LOG_SEVERITY_NONE)
                .value("count", RS2_LOG_SEVERITY_COUNT);

    m.def("log_to_console", &rs2::log_to_console, "min_severity"_a);
    m.def("log_to_file", &rs2::log_to_file, "min_severity"_a, "file_path"_a);
    
    return m.ptr();
}
