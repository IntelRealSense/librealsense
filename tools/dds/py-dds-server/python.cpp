/* License: Apache 2.0. See LICENSE file in root directory.
Copyright(c) 2017 Intel Corporation. All Rights Reserved. */

#include "python.hpp"

#include <librealsense2/dds/dds-participant.h>
#include <librealsense2/utilities/easylogging/easyloggingpp.h>
//#include <librealsense2/rs.h>
#include <fastdds/dds/log/Log.hpp>


INITIALIZE_EASYLOGGINGPP


PYBIND11_MODULE(NAME, m) {
    m.doc() = R"pbdoc(
        RealSense DDS Server Python Bindings
    )pbdoc";
    m.attr( "__version__" ) = "0.1";  // RS2_API_VERSION_STR;

    el::Configurations defaultConf;
    defaultConf.setToDefault();
    defaultConf.setGlobally( el::ConfigurationType::ToStandardOutput, "true" );
    defaultConf.setGlobally( el::ConfigurationType::Format, " %datetime{%d/%M %H:%m:%s,%g} %level [%thread] (%fbase:%line) %msg" );
    el::Loggers::reconfigureLogger( "librealsense", defaultConf );

    m.def( "debug",
        []( bool enable = true ) {
        struct log_consumer : eprosima::fastdds::dds::LogConsumer
        {
            virtual void Consume( const eprosima::fastdds::dds::Log::Entry & e ) override
            {
                using eprosima::fastdds::dds::Log;
                switch( e.kind )
                {
                case Log::Kind::Error:
                    LOG_ERROR( "[DDS] " << e.message );
                    break;
                case Log::Kind::Warning:
                    LOG_WARNING( "[DDS] " << e.message );
                    break;
                case Log::Kind::Info:
                    LOG_DEBUG( "[DDS] " << e.message );
                    break;
                }
            }
        };

        if( enable )
        {
            //rs2_log_to_console( RS2_LOG_SEVERITY_DEBUG, nullptr );
            eprosima::fastdds::dds::Log::SetVerbosity( eprosima::fastdds::dds::Log::Info );
        }
        else
        {
            eprosima::fastdds::dds::Log::SetVerbosity( eprosima::fastdds::dds::Log::Error );
            //rs2::log_to_console( RS2_LOG_SEVERITY_ERROR );
        }


        }
        );

    py::class_< librealsense::dds::dds_participant > participant( m, "participant" );
    participant.def( py::init<>() )
        .def( "init", &librealsense::dds::dds_participant::init, "domain-id"_a, "participant-name"_a );


}
