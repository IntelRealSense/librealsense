// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <utilities/py/pybind11.h>
#include <utilities/easylogging/easyloggingpp.h>
#include <utilities/string/split.h>

#define NAME pyrsutils
#define SNAME "pyrsutils"


#ifndef BUILD_SHARED_LIBS  // shared-init takes care of the else
INITIALIZE_EASYLOGGINGPP
#endif


PYBIND11_MODULE(NAME, m) {
    m.doc() = R"pbdoc(
        RealSense Utilities Python Bindings
    )pbdoc";
    m.attr( "__version__" ) = "0.1";  // RS2_API_VERSION_STR;

    // Configure the same logger as librealsense, and default to only errors by default...
    el::Configurations defaultConf;
    defaultConf.setToDefault();
    defaultConf.setGlobally( el::ConfigurationType::ToStandardOutput, "false" );
    defaultConf.set( el::Level::Error, el::ConfigurationType::ToStandardOutput, "true" );
    defaultConf.setGlobally( el::ConfigurationType::Format, "-%levshort- %datetime{%H:%m:%s.%g} %msg (%fbase:%line [%thread])" );
    el::Loggers::reconfigureLogger( "librealsense", defaultConf );

    m.def(
        "debug",
        []( bool enable, std::string const & nested ) {
            el::Logger * logger = el::Loggers::getLogger( "librealsense" );
            auto configs = logger->configurations();
            configs->set( el::Level::Warning, el::ConfigurationType::ToStandardOutput, enable ? "true" : "false" );
            configs->set( el::Level::Info, el::ConfigurationType::ToStandardOutput, enable ? "true" : "false" );
            configs->set( el::Level::Debug, el::ConfigurationType::ToStandardOutput, enable ? "true" : "false" );
            std::string format = "-%levshort- %datetime{%H:%m:%s.%g} %msg (%fbase:%line [%thread])";
            if( ! nested.empty() )
                format = '[' + nested + "] " + format;
            configs->setGlobally( el::ConfigurationType::Format, format );
            logger->reconfigure();
        },
        py::arg( "enable" ),
        py::arg( "nested-string" ) = "" );

    m.def( "split", &utilities::string::split );
}
