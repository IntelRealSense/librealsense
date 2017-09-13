#include <pybind11/pybind11.h>

// convenience functions
#include <pybind11/operators.h>

// STL conversions
#include <pybind11/stl.h>

// makes certain STL containers opaque to prevent expensive copies
#include <pybind11/stl_bind.h>

// makes std::function conversions work
#include <pybind11/functional.h>

#include "../include/librealsense2/rs.h"
#include "../include/librealsense2/rs.hpp"

#define NAME pyrealsense2
#define SNAME "pyrealsense2"
#define BIND_RAW_ARRAY(class, name, type, size) #name, [](const class &c) -> const std::array<type, size>& { return reinterpret_cast<const std::array<type, size>&>(c.name); }
// hacky little bit of half-functions to make .def(BIND_DOWNCAST) look nice for binding as/is functions
#define BIND_DOWNCAST(class, downcast) "is_"#downcast, &rs2::class::is<rs2::downcast>).def("as_"#downcast, &rs2::class::as<rs2::downcast>

/*PYBIND11_MAKE_OPAQUE(std::vector<rs2::stream_profile>)*/

namespace py = pybind11;
using namespace pybind11::literals;

PYBIND11_PLUGIN(NAME) {
    py::module m(SNAME, "Library for accessing Intel RealSenseTM cameras");

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
        BufData(void *ptr, size_t itemsize, const std::string &format, size_t size)
            : BufData(ptr, itemsize, format, 1, std::vector<size_t> { size }, std::vector<size_t> { itemsize }) { }
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

    /* enums, c-structs */
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

    py::enum_<rs2_camera_info> camera_info(m, "camera_info");
    camera_info.value("name", RS2_CAMERA_INFO_NAME)
               .value("serial_number", RS2_CAMERA_INFO_SERIAL_NUMBER)
               .value("firmware_version", RS2_CAMERA_INFO_FIRMWARE_VERSION)
               .value("location", RS2_CAMERA_INFO_PHYSICAL_PORT)
               .value("debug_op_code", RS2_CAMERA_INFO_DEBUG_OP_CODE)
               .value("advanced_mode", RS2_CAMERA_INFO_ADVANCED_MODE)
               .value("product_id", RS2_CAMERA_INFO_PRODUCT_ID)
               .value("camera_locked", RS2_CAMERA_INFO_CAMERA_LOCKED)
               .value("count", RS2_CAMERA_INFO_COUNT);

    py::enum_<rs2_frame_metadata_value> frame_metadata(m, "frame_metadata");
    frame_metadata.value("frame counter", RS2_FRAME_METADATA_FRAME_COUNTER)
                  .value("frame_timestamp", RS2_FRAME_METADATA_FRAME_TIMESTAMP)
                  .value("sensor_timestamp", RS2_FRAME_METADATA_SENSOR_TIMESTAMP)
                  .value("actual_exposure", RS2_FRAME_METADATA_ACTUAL_EXPOSURE)
                  .value("gain_level", RS2_FRAME_METADATA_GAIN_LEVEL)
                  .value("auto_exposure", RS2_FRAME_METADATA_AUTO_EXPOSURE)
                  .value("white_balance", RS2_FRAME_METADATA_WHITE_BALANCE)
                  .value("time_of_arrival", RS2_FRAME_METADATA_TIME_OF_ARRIVAL)
                  .value("count", RS2_FRAME_METADATA_COUNT);

    py::enum_<rs2_stream> stream(m, "stream");
    stream.value("any", RS2_STREAM_ANY)
        .value("depth", RS2_STREAM_DEPTH)
        .value("color", RS2_STREAM_COLOR)
        .value("infrared", RS2_STREAM_INFRARED)
        .value("fisheye", RS2_STREAM_FISHEYE)
        .value("gyro", RS2_STREAM_GYRO)
        .value("accel", RS2_STREAM_ACCEL)
        .value("gpio", RS2_STREAM_GPIO)
        .value("count", RS2_STREAM_COUNT);

    py::enum_<rs2_extension> extension(m, "extension");
    extension.value("unknown", RS2_EXTENSION_UNKNOWN)
             .value("video_frame", RS2_EXTENSION_VIDEO_FRAME)
             .value("depth_frame", RS2_EXTENSION_DEPTH_FRAME)
             .value("count", RS2_EXTENSION_COUNT);

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
          .value("gpio_raw", RS2_FORMAT_GPIO_RAW)
          .value("count", RS2_FORMAT_COUNT);

    py::class_<rs2_intrinsics> intrinsics(m, "intrinsics");
    intrinsics.def_readonly("width", &rs2_intrinsics::width)
              .def_readonly("height", &rs2_intrinsics::height)
              .def_readonly("ppx", &rs2_intrinsics::ppx)
              .def_readonly("ppy", &rs2_intrinsics::ppy)
              .def_readonly("fx", &rs2_intrinsics::fx)
              .def_readonly("fy", &rs2_intrinsics::fy)
              .def_readonly("model", &rs2_intrinsics::model)
              .def_property_readonly(BIND_RAW_ARRAY(rs2_intrinsics, coeffs, float, 5));

    py::enum_<rs2_notification_category> notification_category(m, "notification_category");
    notification_category.value("frames_timeout", rs2_notification_category::RS2_NOTIFICATION_CATEGORY_FRAMES_TIMEOUT)
                         .value("frame_corrupted", rs2_notification_category::RS2_NOTIFICATION_CATEGORY_FRAME_CORRUPTED)
                         .value("hardware_error", rs2_notification_category::RS2_NOTIFICATION_CATEGORY_HARDWARE_ERROR)
                         .value("unknown_error", rs2_notification_category::RS2_NOTIFICATION_CATEGORY_UNKNOWN_ERROR)
                         .value("count", rs2_notification_category::RS2_NOTIFICATION_CATEGORY_COUNT);

    py::enum_<rs2_log_severity> log_severity(m, "log_severity");
    log_severity.value("debug", RS2_LOG_SEVERITY_DEBUG)
                .value("info", RS2_LOG_SEVERITY_INFO)
                .value("warn", RS2_LOG_SEVERITY_WARN)
                .value("error", RS2_LOG_SEVERITY_ERROR)
                .value("fatal", RS2_LOG_SEVERITY_FATAL)
                .value("none", RS2_LOG_SEVERITY_NONE)
                .value("count", RS2_LOG_SEVERITY_COUNT);

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
          .value("auto_exposure_mode", RS2_OPTION_AUTO_EXPOSURE_MODE)
          .value("power_line_frequency", RS2_OPTION_POWER_LINE_FREQUENCY)
          .value("asic_temperature", RS2_OPTION_ASIC_TEMPERATURE)
          .value("error_polling_enabled", RS2_OPTION_ERROR_POLLING_ENABLED)
          .value("projector_temperature", RS2_OPTION_PROJECTOR_TEMPERATURE)
          .value("output_trigger_enabled", RS2_OPTION_OUTPUT_TRIGGER_ENABLED)
          .value("motion_module_temperature", RS2_OPTION_MOTION_MODULE_TEMPERATURE)
          .value("depth_units", RS2_OPTION_DEPTH_UNITS)
          .value("enable_motion_correction", RS2_OPTION_ENABLE_MOTION_CORRECTION)
          .value("auto_exposure_priority", RS2_OPTION_AUTO_EXPOSURE_PRIORITY)
          .value("count", RS2_OPTION_COUNT);

    // Multi-dimensional c-array rs2_motion_device_instrinc::data causing trouble
    //py::class_<rs2_motion_device_intrinsic> motion_device_inrinsic(m, "motion_device_intrinsic");
    //motion_device_inrinsic.def_property_readonly(BIND_RAW_ARRAY(rs2_motion_device_intrinsic, noise_variances, float, 3))
    //                      .def_property_readonly(BIND_RAW_ARRAY(rs2_motion_device_intrinsic, bias_variances, float, 3));

    py::enum_<rs2_timestamp_domain> ts_domain(m, "timestamp_domain");
    ts_domain.value("hardware_clock", RS2_TIMESTAMP_DOMAIN_HARDWARE_CLOCK)
             .value("system_time", RS2_TIMESTAMP_DOMAIN_SYSTEM_TIME)
             .value("count", RS2_TIMESTAMP_DOMAIN_COUNT);

    py::enum_<rs2_distortion> distortion(m, "distortion");
    distortion.value("none", RS2_DISTORTION_NONE)
              .value("modified_brown_conrady", RS2_DISTORTION_MODIFIED_BROWN_CONRADY)
              .value("inverse_brown_conrady", RS2_DISTORTION_INVERSE_BROWN_CONRADY)
              .value("ftheta", RS2_DISTORTION_FTHETA)
              .value("brown_conrady", RS2_DISTORTION_BROWN_CONRADY)
              .value("count", RS2_DISTORTION_COUNT);

    /* rs2_types.hpp */
    // TODO: error types

    // Not binding devices_changed_callback, templated

    // Not binding frame_callback, templated

    py::class_<rs2::option_range> option_range(m, "option_range");
    option_range.def_readwrite("min", &rs2::option_range::min)
                .def_readwrite("max", &rs2::option_range::max)
                .def_readwrite("default", &rs2::option_range::def)
                .def_readwrite("step", &rs2::option_range::step)
                .def("__repr__", [](const rs2::option_range &self){
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
           .def("query_devices", &rs2::context::query_devices, "Create a static"
                " snapshot of all connected devices a the time of the call.")
           .def("query_all_sensors", &rs2::context::query_all_sensors, "Generate a flat list of "
                "all available sensors from all RealSense devices.")
           .def("get_sensor_parent", &rs2::context::get_sensor_parent, "s"_a)
           .def("set_devices_changed_callback", [](rs2::context& self, std::function<void(rs2::event_information)> &callback)
                {
                    self.set_devices_changed_callback(callback);
                }, "Register devices changed callback.", "callback"_a)
           // not binding create_processing_block, not in Python API.
           .def("load_device", &rs2::context::load_device, "Creates a devices from a RealSense file.\n"
                "On successful load, the device will be appended to the context and a devices_changed event triggered."
                "filename"_a)
           .def("unload_device", &rs2::context::unload_device, "filename"_a);

    /* rs2_device.hpp */
    py::class_<rs2::device> device(m, "device");
    device.def("query_sensors", &rs2::device::query_sensors, "Returns the list of adjacent devices, "
               "sharing the same physical parent composite device.")
          .def("first_depth_sensor", [](rs2::device& self){ return self.first<rs2::depth_sensor>(); })
          .def("first_roi_sensor", [](rs2::device& self) { return self.first<rs2::roi_sensor>(); })
          .def("supports", &rs2::device::supports, "Check if specific camera info is supported.", "info"_a)
          .def("get_info", &rs2::device::get_info, "Retrieve camera specific information, "
               "like versions of various internal components", "info"_a)
          .def("hardware_reset", &rs2::device::hardware_reset, "Send hardware reset request to the device")
          .def(py::init<>())
          .def("__nonzero__", &rs2::device::operator bool)
          .def(BIND_DOWNCAST(device, debug_protocol))
          .def(BIND_DOWNCAST(device, playback))
          .def(BIND_DOWNCAST(device, recorder))
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
               .def("__getitem__", [](const rs2::device_list& self, size_t i){
                       if (i >= self.size())
                           throw py::index_error();
                       return self[i];
                })
               .def("__len__", &rs2::device_list::size)
               .def("size", &rs2::device_list::size)
               .def("__iter__", [](const rs2::device_list& self){
                       return py::make_iterator(self.begin(), self.end());
                   }, py::keep_alive<0, 1>())
               .def("__getitem__", [](const rs2::device_list& self, py::slice slice){
                       size_t start, stop, step, slicelength;
                       if (!slice.compute(self.size(), &start, &stop, &step, &slicelength))
                           throw py::error_already_set();
                       auto *dlist = new std::vector<rs2::device>(slicelength);
                       for (size_t i = 0; i < slicelength; ++i) {
                           (*dlist)[i] = self[start];
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
                     .def("get_new_devices", &rs2::event_information::get_new_devices, "Returns a "
                          "list of all newly connected devices");

    /* rs2_frame.hpp */
    py::class_<rs2::frame> frame(m, "frame");
    frame.def(py::init<>())
//         .def(py::self = py::self) // can't overload assignment in python
         .def(py::init<rs2::frame>())
         .def("swap", &rs2::frame::swap, "other"_a)
         .def("__nonzero__", &rs2::frame::operator bool)
         .def("get_timestamp", &rs2::frame::get_timestamp, "Retrieve the time at which the frame was captured")
         .def("get_frame_timestamp_domain", &rs2::frame::get_frame_timestamp_domain,
              "Retrieve the timestamp domain.")
         .def("get_frame_metadata", &rs2::frame::get_frame_metadata, "Retrieve the current value "
              "of a single frame_metadata.", "frame_metadata"_a)
         .def("supports_frame_metadata", &rs2::frame::supports_frame_metadata, "Determine if the device "
              "allows a specific metadata to be queried.", "frame_metadata"_a)
         .def("get_frame_number", &rs2::frame::get_frame_number, "Retrieve the frame number.")
         .def("get_data", [](const rs2::frame& self) ->  BufData
              {
                  if (auto vf = self.as<rs2::video_frame>())
                      return BufData(const_cast<void*>(vf.get_data()), 1, std::string("@B"), 2,
                                             { static_cast<size_t>(vf.get_height()), static_cast<size_t>(vf.get_width()) },
                                             { static_cast<size_t>(vf.get_stride_in_bytes()), 1 });
                  else
                      return BufData(const_cast<void*>(self.get_data()), 1, std::string("@B"), 0); },
              "retrieve data from the frame handle.", py::keep_alive<0, 1>())
         .def("get_profile", &rs2::frame::get_profile)
         .def(BIND_DOWNCAST(frame, frame))
         .def(BIND_DOWNCAST(frame, points))
         .def(BIND_DOWNCAST(frame, frameset))
         .def(BIND_DOWNCAST(frame, video_frame))
         .def(BIND_DOWNCAST(frame, depth_frame));

    py::class_<rs2::video_frame, rs2::frame> video_frame(m, "video_frame");
    video_frame.def(py::init<rs2::frame>())
                .def("get_width", &rs2::video_frame::get_width, "Returns image width in pixels.")
                .def("get_height", &rs2::video_frame::get_height, "Returns image height in pixels.")
                .def("get_stride_in_bytes", &rs2::video_frame::get_stride_in_bytes, "Retrieve frame stride, meaning the actual line width in memory in bytes (not the logical image width).")
                .def("get_bits_per_pixel", &rs2::video_frame::get_bits_per_pixel, "Retrieve bits per pixel.")
                .def("get_bytes_per_pixel", &rs2::video_frame::get_bytes_per_pixel, "Retrieve bytes per pixel.");

    py::class_<rs2::vertex> vertex(m, "vertex");
    vertex.def_readwrite("x", &rs2::vertex::x)
          .def_readwrite("y", &rs2::vertex::y)
          .def_readwrite("z", &rs2::vertex::z)
          .def("__init__", [](rs2::vertex &instance) { new (&instance) rs2::vertex;})
          .def("__init__", [](rs2::vertex &instance, float x, float y, float z)
               {
                    new (&instance) rs2::vertex{x,y,z};
               }, "x"_a, "y"_a, "z"_a);

    py::class_<rs2::texture_coordinate> texture_coordinate(m, "texture_coordinate");
    texture_coordinate.def_readwrite("u", &rs2::texture_coordinate::u)
                      .def_readwrite("v", &rs2::texture_coordinate::v)
                      .def("__init__", [](rs2::texture_coordinate &instance) { new (&instance) rs2::vertex; })
                      .def("__init__", [](rs2::texture_coordinate &instance, float u, float v)
                           {
                                new (&instance) rs2::texture_coordinate{ u,v };
                           }, "u"_a, "v"_a);

    py::class_<rs2::points, rs2::frame> points(m, "points");
    points.def(py::init<>())
          .def(py::init<rs2::frame>())
          .def("get_vertices", [](rs2::points& self) -> BufData
               {
                   return BufData(const_cast<rs2::vertex*>(self.get_vertices()),
                                          sizeof(rs2::vertex), std::string("@fff"), self.size());
               }, py::keep_alive<0, 1>())
          .def("get_texture_coordinates", [](rs2::points& self) -> BufData
               {
                   return BufData(const_cast<rs2::texture_coordinate*>(self.get_texture_coordinates()),
                                          sizeof(rs2::texture_coordinate), std::string("@ff"), self.size());
               }, py::keep_alive<0, 1>())
          .def("size", &rs2::points::size);

    py::class_<rs2::frameset, rs2::frame> frameset(m, "composite_frame");
    frameset.def(py::init<rs2::frame>())
                   .def("first_or_default", &rs2::frameset::first_or_default, "s"_a)
                   .def("first", &rs2::frameset::first, "s"_a)
                   .def("size", &rs2::frameset::size)
                   .def("foreach", [](const rs2::frameset& self, std::function<void(rs2::frame)> callable)
                        {
                            self.foreach(callable);
                        })
                   .def("__getitem__", &rs2::frameset::operator[])
                   .def("get_depth_frame", &rs2::frameset::get_depth_frame)
                   .def("get_color_frame", &rs2::frameset::get_color_frame)
                   .def("__iter__", [](rs2::frameset& self)
                    {
                        return py::make_iterator(self.begin(), self.end());
                    }, py::keep_alive<0, 1>())
                   .def("size", &rs2::frameset::size)
                   .def("__getitem__", &rs2::frameset::operator[]);

    py::class_<rs2::frame_source> frame_source(m, "frame_source");
    frame_source.def("allocate_video_frame", &rs2::frame_source::allocate_video_frame,
                     "profile"_a, "original"_a, "new_bpp"_a=0, "new_width"_a=0,
                     "new_height"_a=0, "new_stride"_a=0, "frame_type"_a=RS2_EXTENSION_VIDEO_FRAME)
                .def("allocate_composite_frame", &rs2::frame_source::allocate_composite_frame,
                     "frames"_a) // does anything special need to be done for the vector argument?
                .def("frame_ready", &rs2::frame_source::frame_ready, "result"_a);

    py::class_<rs2::depth_frame, rs2::video_frame> depth_frame(m, "depth_frame");
    depth_frame.def(py::init<rs2::frame>())
               .def("get_distance", &rs2::depth_frame::get_distance, "x"_a, "y"_a);




    /* rs2_processing.hpp */
    // Not binding frame_processor_callback, templated
    py::class_<rs2::processing_block> processing_block(m, "processing_block");
    processing_block.def("start", [](rs2::processing_block& self, std::function<void(rs2::frame)> f)
                            {
                                self.start(f);
                            }, "callback"_a)
                    .def("invoke", &rs2::processing_block::invoke, "f"_a)
                    /*.def("__call__", &rs2::processing_block::operator(), "f"_a)*/;

    // Not binding syncer_processing_block, not in Python API

    py::class_<rs2::frame_queue> frame_queue(m, "frame_queue");
    frame_queue.def(py::init<unsigned int>(), "Create a frame queue. Frame queues are the simplest "
                    "cross-platform synchronization primitive provided by librealsense to help "
                    "developers who are not using async APIs.")
               .def(py::init<>())
        .def("wait_for_frame", [](const rs2::frame_queue& self, unsigned int timeout_ms) { py::gil_scoped_release(); self.wait_for_frame(timeout_ms); }, "Wait until a new frame "
                    "becomes available in the queue and dequeue it.", "timeout_ms"_a=5000)
               .def("poll_for_frame", [](const rs2::frame_queue &self)
                    {
                        rs2::frame frame;
                        self.poll_for_frame(&frame);
                        return frame;
                    }, "Poll if a new frame is available and dequeue it if it is")
               .def("__call__", &rs2::frame_queue::operator());

    py::class_<rs2::pointcloud> pointcloud(m, "pointcloud");
    pointcloud.def("calculate", &rs2::pointcloud::calculate, "depth"_a)
              .def("map_to", &rs2::pointcloud::map_to, "mapped"_a);

    py::class_<rs2::syncer> syncer(m, "syncer");
    syncer.def(py::init<>())
          .def("wait_for_frames", &rs2::syncer::wait_for_frames, "Wait until a coherent set "
               "of frames becomes available", "timeout_ms"_a = 5000)
          .def("poll_for_frames", [](const rs2::syncer &self)
               {
                   rs2::frameset frames;
                   self.poll_for_frames(&frames);
                   return frames;
               }, "Check if a coherent set of frames is available");
          /*.def("__call__", &rs2::syncer::operator(), "frame"_a)*/;

    py::class_<rs2::colorizer> colorizer(m, "colorizer");
    colorizer.def(py::init<>())
             .def("colorize", &rs2::colorizer::colorize, "depth"_a)
             /*.def("__call__", &rs2::colorizer::operator())*/;

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
                  .def("stream_name", &rs2::stream_profile::stream_name)
                  .def("size", &rs2::stream_profile::size)
                  .def("__nonzero__", &rs2::stream_profile::operator bool)
                  .def("get_extrinsics_to", &rs2::stream_profile::get_extrinsics_to, "to"_a)
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
                        .def("__repr__", [](const rs2::video_stream_profile& self)
                             {
                                 std::stringstream ss;
                                 ss << "<" SNAME ".video_stream_profile: "
                                    << self.stream_type() << "(" << self.stream_index() << ") " << self.width()
                                    << "x" << self.height() << " @ " << self.fps() << "fps "
                                    << self.format() << ">";
                                 return ss.str();
                             });

    py::class_<rs2::notification> notification(m, "notification");
    notification.def(py::init<>())
        .def("get_category", &rs2::notification::get_category,
            "Retrieve the notification's category.")
        .def("get_description", &rs2::notification::get_description,
            "Retrieve the notification's description.")
        .def("get_timestamp", &rs2::notification::get_timestamp,
            "Retrieve the notification's arrival timestamp.")
        .def("get_severity", &rs2::notification::get_severity,
            "Retrieve the notification's severity.");

    // not binding notifications_callback, templated

    py::class_<rs2::sensor> sensor(m, "sensor");
    sensor.def("open", (void (rs2::sensor::*)(const rs2::stream_profile&) const) &rs2::sensor::open,
               "Open subdevice for exclusive access, by commiting to a configuration", "profile"_a)
          .def("supports", (bool (rs2::sensor::*)(rs2_camera_info) const) &rs2::device::supports,
               "Check if speific camera info is supported.", "info")
          .def("get_info", &rs2::sensor::get_info, "Retrieve camera specific information, "
               "like versions of various internal components.", "info"_a)
          .def("open", (void (rs2::sensor::*)(const std::vector<rs2::stream_profile>&) const) &rs2::sensor::open,
               "Open subdevice for exclusive access, by committing to a composite configuration, specifying one or "
               "more stream profiles.", "profiles"_a)
          .def("close", [](const rs2::sensor& self){ py::gil_scoped_release lock; self.close(); }, "Close subdevice for exclusive access.")
          .def("start", [](const rs2::sensor& self, std::function<void(rs2::frame)> callback)
               { self.start(callback); }, "Start passing frames into user provided callback.", "callback"_a)
          .def("start", [](const rs2::sensor& self, rs2::frame_queue& queue) { self.start(queue); })
          .def("stop", &rs2::sensor::stop, "Stop streaming.")
          .def("is_option_read_only", &rs2::sensor::is_option_read_only, "Check if particular option "
               "is read only.", "option"_a)
          .def("set_notifications_callback", [](const rs2::sensor& self, std::function<void(rs2::notification)> callback)
               { self.set_notifications_callback(callback); }, "Register Notifications callback", "callback"_a)
          .def("get_option", &rs2::sensor::get_option, "Read option value from the device.", "option"_a)
          .def("get_option_range", &rs2::sensor::get_option_range, "Retrieve the available range of values "
               "of a supported option", "option"_a)
          .def("set_option", &rs2::sensor::set_option, "Write new value to device option", "option"_a, "value"_a)
          .def("supports", (bool (rs2::sensor::*)(rs2_option option) const) &rs2::sensor::supports, "Check if particular "
               "option is supported by a subdevice", "option"_a)
          .def("get_option_description", &rs2::sensor::get_option_description, "Get option description.", "option"_a)
          .def("get_option_value_description", &rs2::sensor::get_option_value_description, "Get option value description "
               "(In case a specific option value holds special meaning)", "option"_a, "value"_a)
          .def("get_stream_profiles", &rs2::sensor::get_stream_profiles, "Check if physical subdevice is supported.")
          .def("get_motion_intrisics", &rs2::sensor::get_motion_intrinsics, "Returns scale and bias of a motion stream.",
               "stream"_a)
          .def(py::init<>())
          .def("__nonzero__", &rs2::sensor::operator bool)
          .def(BIND_DOWNCAST(sensor, roi_sensor))
          .def(BIND_DOWNCAST(sensor, depth_sensor));

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

    /* rs2_pipeline.hpp */
    py::class_<rs2::pipeline> pipeline(m, "pipeline");
    pipeline.def(py::init<rs2::context>(), "ctx"_a = rs2::context())
            .def("get_device", &rs2::pipeline::get_device)
            .def("start", (void (rs2::pipeline::*)()const) &rs2::pipeline::start)
            .def("stop", &rs2::pipeline::stop)
            .def("enable_stream" , &rs2::pipeline::enable_stream, "stream"_a, "index"_a,
                 "width"_a, "height"_a, "format"_a, "framerate"_a)
            .def("disable_stream", &rs2::pipeline::disable_stream, "stream"_a)
            .def("disable_all", &rs2::pipeline::disable_all)
            .def("wait_for_frames", &rs2::pipeline::wait_for_frames, "timeout_ms"_a = 5000)
            .def("poll_for_frames", &rs2::pipeline::poll_for_frames, "frameset*"_a)
            .def("enable_device",  &rs2::pipeline::enable_device,  "std::string"_a)
            .def("get_active_streams", (rs2::stream_profile (rs2::pipeline::*)(const rs2_stream, const int) const)
                 &rs2::pipeline::get_active_streams, "stream"_a, "index"_a = 0)
            .def("get_active_streams", (std::vector<rs2::stream_profile> (rs2::pipeline::*)() const)
                 &rs2::pipeline::get_active_streams)
            .def("open", &rs2::pipeline::open);

    /* rs2.hpp */
    m.def("log_to_console", &rs2::log_to_console, "min_severity"_a);
    m.def("log_to_file", &rs2::log_to_file, "min_severity"_a, "file_path"_a);

    return m.ptr();
}
