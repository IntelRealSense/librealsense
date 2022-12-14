/* License: Apache 2.0. See LICENSE file in root directory.
Copyright(c) 2020 Intel Corporation. All Rights Reserved. */

#include <pybind11/pybind11.h>

// convenience functions
#include <pybind11/operators.h>

#include "librealsense2-net/rs_net.hpp"

#define NAME pyrealsense2_net
#define SNAME "pyrealsense2_net"

namespace py = pybind11;
using namespace pybind11::literals;

auto device = (py::object) py::module::import("pyrealsense2").attr("device");

PYBIND11_MODULE(NAME, m) {
    m.doc() = "Wrapper for the librealsense ethernet device extension module";

    py::class_<rs2::net_device> net_device(m, "net_device", device);
    net_device.def(py::init<std::string>(), "address"_a)
        .def("add_to", &rs2::net_device::add_to, "Add net device to existing context.\n"
             "Any future queries on the context will return this device.\n"
             "This operation cannot be undone (except for destroying the context)", "ctx"_a);
}
