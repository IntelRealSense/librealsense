#include <pybind11/pybind11.h>

// convenience functions
#include <pybind11/operators.h>

// STL conversions
#include <pybind11/stl.h>

#include "../src/backend.h"

#define NAME pybackend2
#define SNAME "pybackend2"

namespace py = pybind11;
using namespace pybind11::literals;

using namespace rsimpl2;

PYBIND11_PLUGIN(NAME) {
    py::module m(SNAME, "Bindings for the backend of librealsense"); // equivalent to rsimpl2 namespace
    auto uvc_m = m.def_submodule("uvc", "uvc namespace");

    py::class_<uvc::time_service> time_service(uvc_m, "time_service");
    time_service.def("get_time", &uvc::time_service::get_time);

    py::class_<uvc::os_time_service, uvc::time_service> os_time_service(uvc_m, "os_time_service");

#define BIND_RAW_ARRAY(class, name, type, size) #name, [](const class &c) -> const std::array<type, size>& { return reinterpret_cast<const std::array<type, size>&>(c.name); }

    py::class_<uvc::guid> guid(uvc_m, "guid");
    guid.def_readwrite("data1", &uvc::guid::data1)
        .def_readwrite("data2", &uvc::guid::data2)
        .def_readwrite("data3", &uvc::guid::data3)
        .def_property(BIND_RAW_ARRAY(uvc::guid, data4, uint8_t, 8), [](uvc::guid &g, const std::array<uint8_t, 8> &arr)
            {
                for (int i=0; i<8; ++i) g.data4[i] = arr[i];
        });

    py::class_<uvc::extension_unit> extension_unit(uvc_m, "extension_unit");
    extension_unit.def_readwrite("subdevice", &uvc::extension_unit::subdevice)
                  .def_readwrite("unit", &uvc::extension_unit::unit)
                  .def_readwrite("node", &uvc::extension_unit::node)
                  .def_readwrite("id", &uvc::extension_unit::id);

    py::class_<uvc::command_transfer> command_transfer(uvc_m, "command_transfer");
    command_transfer.def("send_receive", &uvc::command_transfer::send_receive, "data"_a, "timeout_ms"_a, "require_response"_a);

    py::class_ <uvc::control_range> control_range(uvc_m, "control_range");
    control_range.def_readwrite("min", &uvc::control_range::min)
                 .def_readwrite("max", &uvc::control_range::max)
                 .def_readwrite("def", &uvc::control_range::def)
                 .def_readwrite("step", &uvc::control_range::step);

    py::enum_<uvc::power_state> power_state(uvc_m, "power_state");
    power_state.value("D0", uvc::power_state::D0)
               .value("D3", uvc::power_state::D3);
    power_state.export_values();

    py::class_<uvc::stream_profile> stream_profile(uvc_m, "stream_profile");
    stream_profile.def_readwrite("width", &uvc::stream_profile::width)
                  .def_readwrite("height", &uvc::stream_profile::height)
                  .def_readwrite("fps", &uvc::stream_profile::fps)
                  .def_readwrite("format", &uvc::stream_profile::format)
//                  .def("stream_profile_tuple", &uvc::stream_profile::stream_profile_tuple) // converstion operator to std::tuple
                  .def(py::self == py::self);

    py::class_<uvc::frame_object> frame_object(uvc_m, "frame_object");
    frame_object.def_readwrite("frame_size", &uvc::frame_object::frame_size)
                .def_readwrite("metadata_size", &uvc::frame_object::metadata_size)
                /* How to do pointers?
                .def_property("pixels", [](const uvc::frame_object &f) -> python::data_buffer { return ??? }, ???)
                .def_property("metadata", [](const uvc::frame_object &f) -> python::data_buffer { return ??? }, ???) */;

    py::class_<uvc::uvc_device_info> uvc_device_info(uvc_m, "uvc_device_info");
    uvc_device_info.def_readwrite("id", &uvc::uvc_device_info::id, "To distinguish between different pins of the same device.")
                   .def_readwrite("vid", &uvc::uvc_device_info::vid)
                   .def_readwrite("pid", &uvc::uvc_device_info::pid)
                   .def_readwrite("mi", &uvc::uvc_device_info::mi)
                   .def_readwrite("unique_id", &uvc::uvc_device_info::unique_id)
                   .def_readwrite("device_path", &uvc::uvc_device_info::device_path)
                   .def(py::self == py::self);

    py::class_<uvc::usb_device_info> usb_device_info(uvc_m, "usb_device_info");
    usb_device_info.def_readwrite("id", &uvc::usb_device_info::id)
                   .def_readwrite("vid", &uvc::usb_device_info::vid)
                   .def_readwrite("pid", &uvc::usb_device_info::pid)
                   .def_readwrite("mi", &uvc::usb_device_info::mi)
                   .def_readwrite("unique_id", &uvc::usb_device_info::unique_id);

    py::class_<uvc::hid_device_info> hid_device_info(uvc_m, "hid_device_info");
    hid_device_info.def_readwrite("id", &uvc::hid_device_info::id)
                   .def_readwrite("vid", &uvc::hid_device_info::vid)
                   .def_readwrite("pid", &uvc::hid_device_info::pid)
                   .def_readwrite("unique_id", &uvc::hid_device_info::unique_id)
                   .def_readwrite("device_path", &uvc::hid_device_info::device_path);

    py::class_<uvc::hid_sensor> hid_sensor(uvc_m, "hid_sensor");
    //hid_sensor.def_readwrite("iio", &uvc::hid_sensor::iio, "Industrial I/O - An "
    //                         "index that represents a single hardware sensor")
    //          .def_readwrite("name", &uvc::hid_sensor::name);

    py::class_<uvc::hid_sensor_input> hid_sensor_input(uvc_m, "hid_sensor_input");
    hid_sensor_input.def_readwrite("index", &uvc::hid_sensor_input::index)
                    .def_readwrite("name", &uvc::hid_sensor_input::name);

    py::class_<uvc::callback_data> callback_data(uvc_m, "callback_data");
    callback_data.def_readwrite("sensor", &uvc::callback_data::sensor)
                 .def_readwrite("sensor_input", &uvc::callback_data::sensor_input)
                 .def_readwrite("value", &uvc::callback_data::value);

    py::class_<uvc::sensor_data> sensor_data(uvc_m, "sensor_data");
    sensor_data.def_readwrite("sensor", &uvc::sensor_data::sensor)
               .def_readwrite("fo", &uvc::sensor_data::fo);

    //py::class_<uvc::iio_profile> iio_profile(uvc_m, "iio_profile");
    //iio_profile.def_readwrite("iio", &uvc::iio_profile::iio)
    //    .def_readwrite("frequency", &uvc::iio_profile::frequency);

    py::class_<uvc::hid_device> hid_device(uvc_m, "hid_device");
    hid_device.def("open", &uvc::hid_device::open)
        .def("close", &uvc::hid_device::close)
        .def("stop_capture", &uvc::hid_device::stop_capture)
        .def("start_capture", &uvc::hid_device::start_capture, "iio_profiles"_a, "callback"_a)
        .def("get_sensors", &uvc::hid_device::get_sensors);

    py::class_<uvc::uvc_device> uvc_device(uvc_m, "uvc_device");
    uvc_device.def("probe_and_commit", &uvc::uvc_device::probe_and_commit, "profile"_a, "callback"_a, "buffers"_a)
        .def("stream_on", &uvc::uvc_device::stream_on, "error_handler"_a)
        .def("start_callbacks", &uvc::uvc_device::start_callbacks)
        .def("stop_callbacks", &uvc::uvc_device::stop_callbacks)
        .def("close", &uvc::uvc_device::close, "profile"_a)
        .def("set_power_state", &uvc::uvc_device::set_power_state, "state"_a)
        .def("get_power_state", &uvc::uvc_device::get_power_state)
        .def("init_xu", &uvc::uvc_device::init_xu, "xu"_a)
        .def("set_xu", &uvc::uvc_device::set_xu, "xu"_a, "ctrl"_a, "data"_a, "len"_a)
        .def("get_xu", &uvc::uvc_device::get_xu, "xu"_a, "ctrl"_a, "data"_a, "len"_a)
        .def("get_xu_range", &uvc::uvc_device::get_xu_range, "xu"_a, "ctrl"_a, "len"_a)
        .def("get_pu", &uvc::uvc_device::get_pu, "opt"_a, "value"_a)
        .def("set_pu", &uvc::uvc_device::set_pu, "opt"_a, "value"_a)
        .def("get_pu_range", &uvc::uvc_device::get_pu_range, "opt"_a)
        .def("get_profiles", &uvc::uvc_device::get_profiles)
        .def("lock", &uvc::uvc_device::lock)
        .def("unlock", &uvc::uvc_device::unlock)
        .def("get_device_location", &uvc::uvc_device::get_device_location);

    py::class_<uvc::retry_controls_work_around, uvc::uvc_device> retry_controls_work_around(uvc_m, "retry_controls_work_around");
    retry_controls_work_around.def(py::init<std::shared_ptr<uvc::uvc_device>>());

    py::class_<uvc::usb_device, uvc::command_transfer> usb_device(uvc_m, "usb_device");

    py::class_<uvc::backend> backend(uvc_m, "backend");
    backend.def("create_uvc_device", &uvc::backend::create_uvc_device, "info"_a)
        .def("query_uvc_devices", &uvc::backend::query_uvc_devices)
        .def("create_usb_device", &uvc::backend::create_usb_device, "info"_a)
        .def("query_usb_devices", &uvc::backend::query_usb_devices)
        .def("create_hid_device", &uvc::backend::create_hid_device, "info"_a)
        .def("query_hid_devices", &uvc::backend::query_hid_devices)
        .def("create_time_service", &uvc::backend::create_time_service);

    py::class_<uvc::multi_pins_uvc_device, uvc::uvc_device> multi_pins_uvc_device(uvc_m, "multi_pins_uvc_device");
    multi_pins_uvc_device.def(py::init<std::vector<std::shared_ptr<uvc::uvc_device>>>());

    uvc_m.def("create_backend", &uvc::create_backend);

    return m.ptr();
}