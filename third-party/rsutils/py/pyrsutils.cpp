// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <rsutils/py/pybind11.h>
#include <rsutils/easylogging/easyloggingpp.h>
#include <rsutils/string/split.h>

#define NAME pyrsutils
#define SNAME "pyrsutils"


PYBIND11_MODULE(NAME, m) {
    m.doc() = R"pbdoc(
        RealSense Utilities Python Bindings
    )pbdoc";
    m.attr( "__version__" ) = "0.1";  // RS2_API_VERSION_STR;

    utilities::configure_elpp_logger();

    m.def( "debug",
           &utilities::configure_elpp_logger,
           py::arg( "enable" ),
           py::arg( "nested-string" ) = "",
           py::arg( "logger" ) = LIBREALSENSE_ELPP_ID );

    m.def( "split", &utilities::string::split );
}
