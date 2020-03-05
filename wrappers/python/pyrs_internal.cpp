/* License: Apache 2.0. See LICENSE file in root directory.
Copyright(c) 2017 Intel Corporation. All Rights Reserved. */

#include "python.hpp"
#include "../include/librealsense2/hpp/rs_internal.hpp"
#include <cstring>

void init_internal(py::module &m) {
    /** rs_internal.h **/
    py::class_<rs2_video_stream> video_stream(m, "video_stream", "All the parameters"
                                              " required to define a video stream.");
    video_stream.def(py::init<>())
        .def_readwrite("type", &rs2_video_stream::type)
        .def_readwrite("index", &rs2_video_stream::index)
        .def_readwrite("uid", &rs2_video_stream::uid)
        .def_readwrite("width", &rs2_video_stream::width)
        .def_readwrite("height", &rs2_video_stream::height)
        .def_readwrite("fps", &rs2_video_stream::fps)
        .def_readwrite("bpp", &rs2_video_stream::bpp)
        .def_readwrite("fmt", &rs2_video_stream::fmt)
        .def_readwrite("intrinsics", &rs2_video_stream::intrinsics);

    py::class_<rs2_motion_stream> motion_stream(m, "motion_stream", "All the parameters"
        " required to define a motion stream.");
    motion_stream.def(py::init<>())
        .def_readwrite("type", &rs2_motion_stream::type)
        .def_readwrite("index", &rs2_motion_stream::index)
        .def_readwrite("uid", &rs2_motion_stream::uid)
        .def_readwrite("fps", &rs2_motion_stream::fps)
        .def_readwrite("fmt", &rs2_motion_stream::fmt)
        .def_readwrite("intrinsics", &rs2_motion_stream::intrinsics);

    py::class_<rs2_pose_stream> pose_stream(m, "pose_stream", "All the parameters"
        " required to define a pose stream.");
    pose_stream.def(py::init<>())
        .def_readwrite("type", &rs2_pose_stream::type)
        .def_readwrite("index", &rs2_pose_stream::index)
        .def_readwrite("uid", &rs2_pose_stream::uid)
        .def_readwrite("fps", &rs2_pose_stream::fps)
        .def_readwrite("fmt", &rs2_pose_stream::fmt);

    py::class_<rs2_software_video_frame> software_video_frame(m, "software_video_frame", "All the parameters "
                                                              "required to define a video frame.");
    software_video_frame.def(py::init([]() { rs2_software_video_frame f{}; f.deleter = nullptr; return f; })) // guarantee deleter is set to nullptr
        .def_property("pixels", [](const rs2_software_video_frame& self) {
            // TODO: Not all formats (e.g. RAW10) are properly handled (see struct FmtToType in python.hpp)
            auto vp = rs2::stream_profile(self.profile).as<rs2::video_stream_profile>();
            size_t size = fmt_to_value<itemsize>(vp.format());
            size_t upp = self.bpp / size;
            if (upp == 1)
                return BufData(self.pixels, size, fmt_to_value<fmtstring>(vp.format()), 2,
                    { (size_t)vp.height(), (size_t)vp.width() }, { vp.width()*size, size });
            else
                return BufData(self.pixels, size, fmt_to_value<fmtstring>(vp.format()), 3,
                    { (size_t)vp.height(), (size_t)vp.width(), upp }, { vp.width()*upp*size, upp*size, size });
        }, [](rs2_software_video_frame& self, py::buffer buf) {
            if (self.deleter) self.deleter(self.pixels);
            auto data = buf.request();
            self.pixels = new uint8_t[data.size*data.itemsize]; // fwiw, might be possible to convert data.format -> underlying data type
            std::memcpy(self.pixels, data.ptr, data.size*data.itemsize);
            self.deleter = [](void* ptr){ delete[] ptr; };
        })
        .def_readwrite("stride", &rs2_software_video_frame::stride)
        .def_readwrite("bpp", &rs2_software_video_frame::bpp)
        .def_readwrite("timestamp", &rs2_software_video_frame::timestamp)
        .def_readwrite("domain", &rs2_software_video_frame::domain)
        .def_readwrite("frame_number", &rs2_software_video_frame::frame_number)
        .def_property("profile", [](const rs2_software_video_frame& self) { return rs2::stream_profile(self.profile).as<rs2::video_stream_profile>(); },
                      [](rs2_software_video_frame& self, rs2::video_stream_profile profile) { self.profile = profile.get(); });

    py::class_<rs2_software_motion_frame> software_motion_frame(m, "software_motion_frame", "All the parameters "
                                                                "required to define a motion frame.");
    software_motion_frame.def(py::init([]() { rs2_software_motion_frame f{}; f.deleter = nullptr; return f; })) // guarantee deleter is set to nullptr
        .def_property("data", [](const rs2_software_motion_frame& self) -> rs2_vector {
            auto data = reinterpret_cast<const float*>(self.data);
            return rs2_vector{ data[0], data[1], data[2] };
        }, [](rs2_software_motion_frame& self, rs2_vector data) {
            if (self.deleter) self.deleter(self.data);
            self.data = new float[3];
            float *dptr = reinterpret_cast<float*>(self.data);
            dptr[0] = data.x;
            dptr[1] = data.y;
            dptr[2] = data.z;
            self.deleter = [](void* ptr){ delete[] ptr; };
        })
        .def_readwrite("timestamp", &rs2_software_motion_frame::timestamp)
        .def_readwrite("domain", &rs2_software_motion_frame::domain)
        .def_readwrite("frame_number", &rs2_software_motion_frame::frame_number)
        .def_property("profile", [](const rs2_software_motion_frame& self) { return rs2::stream_profile(self.profile).as<rs2::motion_stream_profile>(); },
                      [](rs2_software_motion_frame& self, rs2::motion_stream_profile profile) { self.profile = profile.get(); });

    py::class_<rs2_software_pose_frame> software_pose_frame(m, "software_pose_frame", "All the parameters "
                                                            "required to define a pose frame.");
    software_pose_frame.def(py::init([]() { rs2_software_pose_frame f{}; f.deleter = nullptr; return f; })) // guarantee deleter is set to nullptr
        .def_property("data", [](const rs2_software_pose_frame& self) -> rs2_pose {
            return *reinterpret_cast<const rs2_pose*>(self.data);
        }, [](rs2_software_pose_frame& self, rs2_pose data) {
            if (self.deleter) self.deleter(self.data);
            self.data = new rs2_pose(data);
            self.deleter = [](void* ptr){ delete[] ptr; };
        })
        .def_readwrite("timestamp", &rs2_software_pose_frame::timestamp)
        .def_readwrite("domain", &rs2_software_pose_frame::domain)
        .def_readwrite("frame_number", &rs2_software_pose_frame::frame_number)
        .def_property("profile", [](const rs2_software_pose_frame& self) { return rs2::stream_profile(self.profile).as<rs2::pose_stream_profile>(); },
                      [](rs2_software_pose_frame& self, rs2::pose_stream_profile profile) { self.profile = profile.get(); });

    py::class_<rs2_software_notification> software_notification(m, "software_notification", "All the parameters "
                                                                "required to define a sensor notification.");
    software_notification.def(py::init<>())
        .def_readwrite("category", &rs2_software_notification::category)
        .def_readwrite("type", &rs2_software_notification::type)
        .def_readwrite("severity", &rs2_software_notification::severity)
        .def_readwrite("description", &rs2_software_notification::description)
        .def_readwrite("serialized_data", &rs2_software_notification::serialized_data);
    /** end rs_internal.h **/
    
    /** rs_internal.hpp **/
    // rs2::software_sensor
    py::class_<rs2::software_sensor> software_sensor(m, "software_sensor");
    software_sensor.def("add_video_stream", &rs2::software_sensor::add_video_stream, "Add video stream to software sensor",
                        "video_stream"_a, "is_default"_a=false)
        .def("add_motion_stream", &rs2::software_sensor::add_motion_stream, "Add motion stream to software sensor",
            "motion_stream"_a, "is_default"_a = false)
        .def("add_pose_stream", &rs2::software_sensor::add_pose_stream, "Add pose stream to software sensor",
            "pose_stream"_a, "is_default"_a = false)
        .def("on_video_frame", &rs2::software_sensor::on_video_frame, "Inject video frame into the sensor", "frame"_a)
        .def("on_motion_frame", &rs2::software_sensor::on_motion_frame, "Inject motion frame into the sensor", "frame"_a)
        .def("on_pose_frame", &rs2::software_sensor::on_pose_frame, "Inject pose frame into the sensor", "frame"_a)
        .def("set_metadata", &rs2::software_sensor::set_metadata, "Set frame metadata for the upcoming frames", "value"_a, "type"_a)
        .def("add_read_only_option", &rs2::software_sensor::add_read_only_option, "Register read-only option that "
             "will be supported by the sensor", "option"_a, "val"_a)
        .def("set_read_only_option", &rs2::software_sensor::set_read_only_option, "Update value of registered "
             "read-only option", "option"_a, "val"_a)
        .def("add_option", &rs2::software_sensor::add_option, "Register option that will be supported by the sensor",
             "option"_a, "range"_a, "is_writable"_a=true)
        .def("on_notification", &rs2::software_sensor::on_notification, "notif"_a);

    // rs2::software_device
    py::class_<rs2::software_device> software_device(m, "software_device");
    software_device.def(py::init<>())
        .def("add_sensor", &rs2::software_device::add_sensor, "Add software sensor with given name "
            "to the software device.", "name"_a)
        .def("set_destruction_callback", &rs2::software_device::set_destruction_callback<std::function<void()>>,
             "Register destruction callback", "callback"_a)
        .def("add_to", &rs2::software_device::add_to, "Add software device to existing context.\n"
             "Any future queries on the context will return this device.\n"
             "This operation cannot be undone (except for destroying the context)", "ctx"_a)
        .def("register_info", &rs2::software_device::register_info, "Add a new camera info value, like serial number",
             "info"_a, "val"_a)
        .def("update_info", &rs2::software_device::update_info, "Update an existing camera info value, like serial number",
             "info"_a, "val"_a);
        //.def("create_matcher", &rs2::software_device::create_matcher, "Set the wanted matcher type that will "
        //     "be used by the syncer", "matcher"_a) // TODO: bind rs2_matchers enum.
    /** end rs_internal.hpp **/
}
