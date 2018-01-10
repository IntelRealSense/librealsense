#include <pybind11/pybind11.h>

// convenience functions
#include <pybind11/operators.h>

// STL conversions
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>

// makes std::function conversions work
#include <pybind11/functional.h>

#include "../src/backend.h"
#include "pybackend_extras.h"
#include "../../third-party/stb_image_write.h"

#include <sstream>
#include <vector>

#define NAME pybackend2
#define SNAME "pybackend2"

namespace py = pybind11;
using namespace pybind11::literals;

using namespace librealsense;
using namespace pybackend2;

// Prevents expensive copies of pixel buffers into python
PYBIND11_MAKE_OPAQUE(std::vector<uint8_t>)

PYBIND11_MODULE(NAME, m) {
    m.doc() = "Wrapper for the backend of librealsense";

    py::class_<platform::control_range> control_range(m, "control_range");
    control_range.def(py::init<>())
                 .def(py::init<int32_t, int32_t, int32_t, int32_t>(), "in_min"_a, "in_max"_a, "in_step"_a, "in_def"_a)
                 .def_readwrite("min", &platform::control_range::min)
                 .def_readwrite("max", &platform::control_range::max)
                 .def_readwrite("def", &platform::control_range::def)
                 .def_readwrite("step", &platform::control_range::step);

    py::class_<platform::time_service> time_service(m, "time_service");
    time_service.def("get_time", &platform::time_service::get_time);

    py::class_<platform::os_time_service, platform::time_service> os_time_service(m, "os_time_service");

#define BIND_RAW_RO_ARRAY(class, name, type, size) #name, [](const class &c) -> const std::array<type, size>& { return reinterpret_cast<const std::array<type, size>&>(c.name); }
#define BIND_RAW_RW_ARRAY(class, name, type, size) BIND_RAW_RO_ARRAY(class, name, type, size), [](class &c, const std::array<type, size> &arr) { for (int i=0; i<size; ++i) c.name[i] = arr[i]; }

    py::class_<platform::guid> guid(m, "guid");
    guid.def_readwrite("data1", &platform::guid::data1)
        .def_readwrite("data2", &platform::guid::data2)
        .def_readwrite("data3", &platform::guid::data3)
        .def_property(BIND_RAW_RW_ARRAY(platform::guid, data4, uint8_t, 8))
        .def("__init__", [](platform::guid &g, uint32_t d1, uint32_t d2, uint32_t d3, std::array<uint8_t, 8> d4)
            {
                new (&g) platform::guid();
                g.data1 = d1;
                g.data2 = d2;
                g.data3 = d3;
                for (int i=0; i<8; ++i) g.data4[i] = d4[i];
            }, "data1"_a, "data2"_a, "data3"_a, "data4"_a)
        .def("__init__", [](platform::guid &g, const std::string &str)
            {
                new (&g) platform::guid(stoguid(str));
            });

    py::class_<platform::extension_unit> extension_unit(m, "extension_unit");
    extension_unit.def(py::init<>())
                  .def("__init__", [](platform::extension_unit & xu, int s, int u, int n, platform::guid g)
                      {
                          new (&xu) platform::extension_unit { s, u, n, g };
                      }, "subdevice"_a, "unit"_a, "node"_a, "guid"_a)
                  .def_readwrite("subdevice", &platform::extension_unit::subdevice)
                  .def_readwrite("unit", &platform::extension_unit::unit)
                  .def_readwrite("node", &platform::extension_unit::node)
                  .def_readwrite("id", &platform::extension_unit::id);

    py::class_<platform::command_transfer> command_transfer(m, "command_transfer");
    command_transfer.def("send_receive", &platform::command_transfer::send_receive, "data"_a, "timeout_ms"_a=5000, "require_response"_a=true);

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
        .value("color_scheme", RS2_OPTION_COLOR_SCHEME)
        .value("histogram_equalization_enabled", RS2_OPTION_HISTOGRAM_EQUALIZATION_ENABLED)
        .value("min_distance", RS2_OPTION_MIN_DISTANCE)
        .value("max_distance", RS2_OPTION_MAX_DISTANCE)
        .value("texture_source", RS2_OPTION_TEXTURE_SOURCE)
        .value("filter_magnitude", RS2_OPTION_FILTER_MAGNITUDE)
        .value("filter_smooth_alpha", RS2_OPTION_FILTER_SMOOTH_ALPHA)
        .value("filter_smooth_delta", RS2_OPTION_FILTER_SMOOTH_DELTA)
        .value("stereo_baseline", RS2_OPTION_STEREO_BASELINE)
        .value("count", RS2_OPTION_COUNT);

    py::enum_<platform::power_state> power_state(m, "power_state");
    power_state.value("D0", platform::power_state::D0)
               .value("D3", platform::power_state::D3);
    power_state.export_values();

    py::class_<platform::stream_profile> stream_profile(m, "stream_profile");
    stream_profile.def_readwrite("width", &platform::stream_profile::width)
                  .def_readwrite("height", &platform::stream_profile::height)
                  .def_readwrite("fps", &platform::stream_profile::fps)
                  .def_readwrite("format", &platform::stream_profile::format)
//                  .def("stream_profile_tuple", &platform::stream_profile::stream_profile_tuple) // converstion operator to std::tuple
                  .def(py::self == py::self).def("__repr__", [](const platform::stream_profile &p) {
        std::stringstream ss;
        ss << "<" SNAME ".stream_profile: "
            << p.width
            << "x" << p.height << " @ " << p.fps << "fps "
            << std::hex << p.format << ">";
        return ss.str();
    });;

    // Bind std::vector<uint8_t> to act like a pythonic list
    py::bind_vector<std::vector<uint8_t>>(m, "VectorByte");

    py::class_<platform::frame_object> frame_object(m, "frame_object");
    frame_object.def_readwrite("frame_size", &platform::frame_object::frame_size)
                .def_readwrite("metadata_size", &platform::frame_object::metadata_size)
                .def_property_readonly("pixels", [](const platform::frame_object &f) { return std::vector<uint8_t>(static_cast<const uint8_t*>(f.pixels), static_cast<const uint8_t*>(f.pixels)+f.frame_size);})
                .def_property_readonly("metadata", [](const platform::frame_object &f) { return std::vector<uint8_t>(static_cast<const uint8_t*>(f.metadata), static_cast<const uint8_t*>(f.metadata)+f.metadata_size);})
                .def("save_png", [](const platform::frame_object &f, std::string fn, int w, int h, int bpp, int s)
                    {
                        stbi_write_png(fn.c_str(), w, h, bpp, f.pixels, s);
                    }, "filename"_a, "width"_a, "height"_a, "bytes_per_pixel"_a, "stride"_a)
                .def("save_png", [](const platform::frame_object &f, std::string fn, int w, int h, int bpp)
                    {
                        stbi_write_png(fn.c_str(), w, h, bpp, f.pixels, w*bpp);
                    }, "filename"_a, "width"_a, "height"_a, "bytes_per_pixel"_a);

    py::class_<platform::uvc_device_info> uvc_device_info(m, "uvc_device_info");
    uvc_device_info.def_readwrite("id", &platform::uvc_device_info::id, "To distinguish between different pins of the same device.")
                   .def_readwrite("vid", &platform::uvc_device_info::vid)
                   .def_readwrite("pid", &platform::uvc_device_info::pid)
                   .def_readwrite("mi", &platform::uvc_device_info::mi)
                   .def_readwrite("unique_id", &platform::uvc_device_info::unique_id)
                   .def_readwrite("device_path", &platform::uvc_device_info::device_path)
                   .def(py::self == py::self);

    py::class_<platform::usb_device_info> usb_device_info(m, "usb_device_info");
    usb_device_info.def_readwrite("id", &platform::usb_device_info::id)
                   .def_readwrite("vid", &platform::usb_device_info::vid)
                   .def_readwrite("pid", &platform::usb_device_info::pid)
                   .def_readwrite("mi", &platform::usb_device_info::mi)
                   .def_readwrite("unique_id", &platform::usb_device_info::unique_id);

    py::class_<platform::hid_device_info> hid_device_info(m, "hid_device_info");
    hid_device_info.def_readwrite("id", &platform::hid_device_info::id)
                   .def_readwrite("vid", &platform::hid_device_info::vid)
                   .def_readwrite("pid", &platform::hid_device_info::pid)
                   .def_readwrite("unique_id", &platform::hid_device_info::unique_id)
                   .def_readwrite("device_path", &platform::hid_device_info::device_path);

    py::class_<platform::hid_sensor> hid_sensor(m, "hid_sensor");
    hid_sensor.def_readwrite("name", &platform::hid_sensor::name);

    py::class_<platform::hid_sensor_input> hid_sensor_input(m, "hid_sensor_input");
    hid_sensor_input.def_readwrite("index", &platform::hid_sensor_input::index)
                    .def_readwrite("name", &platform::hid_sensor_input::name);

    py::class_<platform::callback_data> callback_data(m, "callback_data");
    callback_data.def_readwrite("sensor", &platform::callback_data::sensor)
                 .def_readwrite("sensor_input", &platform::callback_data::sensor_input)
                 .def_readwrite("value", &platform::callback_data::value);

    py::class_<platform::sensor_data> sensor_data(m, "sensor_data");
    sensor_data.def_readwrite("sensor", &platform::sensor_data::sensor)
               .def_readwrite("fo", &platform::sensor_data::fo);

    py::class_<platform::hid_profile> hid_profile(m, "hid_profile");
    hid_profile.def_readwrite("sensor_name", &platform::hid_profile::sensor_name)
               .def_readwrite("frequency", &platform::hid_profile::frequency);

    py::enum_<platform::custom_sensor_report_field> custom_sensor_report_field(m, "custom_sensor_report_field");
    custom_sensor_report_field.value("minimum", platform::custom_sensor_report_field::minimum)
                              .value("maximum", platform::custom_sensor_report_field::maximum)
                              .value("name", platform::custom_sensor_report_field::name)
                              .value("size", platform::custom_sensor_report_field::size)
                              .value("unit_expo", platform::custom_sensor_report_field::unit_expo)
                              .value("units", platform::custom_sensor_report_field::units)
                              .value("value", platform::custom_sensor_report_field::value);
    custom_sensor_report_field.export_values();

    py::class_<platform::hid_sensor_data> hid_sensor_data(m, "hid_sensor_data");
    hid_sensor_data.def_readwrite("x", &platform::hid_sensor_data::x)
                   .def_property(BIND_RAW_RW_ARRAY(platform::hid_sensor_data, reserved1, char, 2))
                   .def_readwrite("y", &platform::hid_sensor_data::y)
                   .def_property(BIND_RAW_RW_ARRAY(platform::hid_sensor_data, reserved2, char, 2))
                   .def_readwrite("z", &platform::hid_sensor_data::z)
                   .def_property(BIND_RAW_RW_ARRAY(platform::hid_sensor_data, reserved3, char, 2))
                   .def_readwrite("ts_low", &platform::hid_sensor_data::ts_low)
                   .def_readwrite("ts_high", &platform::hid_sensor_data::ts_high);

    py::class_<platform::hid_device> hid_device(m, "hid_device");
    hid_device.def("open", &platform::hid_device::open, "hid_profiles"_a)
              .def("close", &platform::hid_device::close)
              .def("stop_capture", &platform::hid_device::stop_capture)
              .def("start_capture", &platform::hid_device::start_capture, "callback"_a)
              .def("get_sensors", &platform::hid_device::get_sensors)
              .def("get_custom_report_data", &platform::hid_device::get_custom_report_data,
                   "custom_sensor_name"_a, "report_name"_a, "report_field"_a);

    py::class_<platform::uvc_device> uvc_device(m, "uvc_device");

    py::class_<platform::retry_controls_work_around, std::shared_ptr<platform::retry_controls_work_around>> retry_controls_work_around(m, "retry_controls_work_around");
    retry_controls_work_around.def(py::init<std::shared_ptr<platform::uvc_device>>())
        .def("probe_and_commit",
            [](platform::retry_controls_work_around& dev, const platform::stream_profile& profile,
                std::function<void(const platform::stream_profile&, const platform::frame_object&)> callback) {
        dev.probe_and_commit(profile, [=](const platform::stream_profile& p,
            const platform::frame_object& fo, std::function<void()> next)
        {
            callback(p, fo);
            next();
        }, 4);
            }
            , "profile"_a, "callback"_a)
        .def("stream_on", [](platform::retry_controls_work_around& dev) {
                dev.stream_on([](const notification& n)
                {
                });
            })
        .def("start_callbacks", &platform::retry_controls_work_around::start_callbacks)
        .def("stop_callbacks", &platform::retry_controls_work_around::stop_callbacks)
        .def("close", [](platform::retry_controls_work_around &dev, platform::stream_profile profile)
            {
                py::gil_scoped_release release;
                dev.close(profile);
            }, "profile"_a)
        .def("set_power_state", &platform::retry_controls_work_around::set_power_state, "state"_a)
        .def("get_power_state", &platform::retry_controls_work_around::get_power_state)
        .def("init_xu", &platform::retry_controls_work_around::init_xu, "xu"_a)
        .def("set_xu", [](platform::retry_controls_work_around &dev, const platform::extension_unit &xu, uint8_t ctrl, py::list l)
            {
                std::vector<uint8_t> data(l.size());
                for (int i = 0; i < l.size(); ++i)
                    data[i] = l[i].cast<uint8_t>();
                dev.set_xu(xu, ctrl, data.data(), (int)data.size());
            }, "xu"_a, "ctrl"_a, "data"_a)
        .def("set_xu", [](platform::retry_controls_work_around &dev, const platform::extension_unit &xu, uint8_t ctrl, std::vector<uint8_t> &data)
            {
                dev.set_xu(xu, ctrl, data.data(), (int)data.size());
            }, "xu"_a, "ctrl"_a, "data"_a)
        .def("get_xu", [](const platform::retry_controls_work_around &dev, const platform::extension_unit &xu, uint8_t ctrl, size_t len)
            {
                std::vector<uint8_t> data(len);
                dev.get_xu(xu, ctrl, data.data(), (int)len);
                py::list ret(len);
                for (size_t i = 0; i < len; ++i)
                    ret[i] = data[i];
                return ret;
            }, "xu"_a, "ctrl"_a, "len"_a)
        .def("get_xu_range", &platform::retry_controls_work_around::get_xu_range, "xu"_a, "ctrl"_a, "len"_a)
        .def("get_pu", [](platform::retry_controls_work_around& dev, rs2_option opt) {
                int val = 0;
                dev.get_pu(opt, val);
                return val;
            }, "opt"_a)
        .def("set_pu", &platform::retry_controls_work_around::set_pu, "opt"_a, "value"_a)
        .def("get_pu_range", &platform::retry_controls_work_around::get_pu_range, "opt"_a)
        .def("get_profiles", &platform::retry_controls_work_around::get_profiles)
        .def("lock", &platform::retry_controls_work_around::lock)
        .def("unlock", &platform::retry_controls_work_around::unlock)
        .def("get_device_location", &platform::retry_controls_work_around::get_device_location);

    py::class_<platform::usb_device, platform::command_transfer, std::shared_ptr<platform::usb_device>> usb_device(m, "usb_device");

    py::class_<platform::backend, std::shared_ptr<platform::backend>> backend(m, "backend");
    backend.def("create_uvc_device", &platform::backend::create_uvc_device, "info"_a)
        .def("query_uvc_devices", &platform::backend::query_uvc_devices)
        .def("create_usb_device", &platform::backend::create_usb_device, "info"_a)
        .def("query_usb_devices", &platform::backend::query_usb_devices)
        .def("create_hid_device", &platform::backend::create_hid_device, "info"_a)
        .def("query_hid_devices", &platform::backend::query_hid_devices)
        .def("create_time_service", &platform::backend::create_time_service);

    py::class_<platform::multi_pins_hid_device> multi_pins_hid_device(m, "multi_pins_hid_device");
    multi_pins_hid_device.def(py::init<std::vector<std::shared_ptr<platform::hid_device>>&>());

    py::class_<platform::multi_pins_uvc_device, platform::uvc_device> multi_pins_uvc_device(m, "multi_pins_uvc_device");
    multi_pins_uvc_device.def(py::init<std::vector<std::shared_ptr<platform::uvc_device>>&>());

    /*py::enum_<command> command_py(m, "command");
    command_py.value("enable_advanced_mode", command::enable_advanced_mode)
              .value("advanced_mode_enabled", command::advanced_mode_enabled)
              .value("reset", command::reset)
              .value("set_advanced", command::set_advanced)
              .value("get_advanced", command::get_advanced);*/

    m.def("create_backend", &platform::create_backend, py::return_value_policy::move);
    m.def("encode_command", [](uint8_t opcode, uint32_t p1, uint32_t p2, uint32_t p3, uint32_t p4, py::list l)
        {
            std::vector<uint8_t> data(l.size());
            for (int i = 0; i < l.size(); ++i)
                data[i] = l[i].cast<uint8_t>();
            return encode_command(static_cast<command>(opcode), p1, p2, p3, p4, data);
        }, "opcode"_a, "p1"_a=0, "p2"_a=0, "p3"_a=0, "p4"_a=0, "data"_a = py::list(0));
}
