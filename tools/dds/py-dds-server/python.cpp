/* License: Apache 2.0. See LICENSE file in root directory.
Copyright(c) 2022 Intel Corporation. All Rights Reserved. */

#include "python.hpp"

#include <librealsense2/dds/dds-participant.h>
#include <librealsense2/utilities/easylogging/easyloggingpp.h>
//#include <librealsense2/rs.h>
#include <fastdds/dds/log/Log.hpp>
#include <fastdds/dds/domain/qos/DomainParticipantQos.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastrtps/types/DynamicType.h>


INITIALIZE_EASYLOGGINGPP


namespace {
std::string to_string( librealsense::dds::dds_guid const & guid )
{
    std::ostringstream os;
    os << guid;
    return os.str();
}
}  // namespace


// "When calling a C++ function from Python, the GIL is always held"
// -- since we're not being called from Python but instead are calling it,
// we need to acquire it to not have issues with other threads...
#define FN_FWD( CLS, FN_NAME, PY_ARGS, FN_ARGS, CODE )                                                                 \
    #FN_NAME,                                                                                                          \
    []( CLS & self, std::function < void PY_ARGS > callback ) {                                                        \
        self.FN_NAME( [callback] FN_ARGS {                                                                             \
            try {                                                                                                      \
                py::gil_scoped_acquire gil;                                                                            \
                CODE                                                                                                   \
            }                                                                                                          \
            catch( ... ) {                                                                                             \
                std::cerr << "?!?!?!!? exception in python " #CLS "." #FN_NAME " ?!?!?!?!?" << std::endl;              \
            }                                                                                                          \
        } );                                                                                                           \
    }


// Convert FastDDS Log::Entry to EasyLogging log (see ELPP_WRITE_LOG)
#define ISNULL(E) ((E) ? (E) : "n/a")
#define LOG_ENTRY( ENTRY, LEVEL, ... )                                                                                 \
    do                                                                                                                 \
    {                                                                                                                  \
        char const * filename = ( ENTRY ).context.filename;                                                            \
        char const * func = ( ENTRY ).context.function;                                                                \
        if( ! func )                                                                                                   \
            func = "n/a";                                                                                              \
        if( ! filename )                                                                                               \
            filename = func;                                                                                           \
        el::base::Writer writer( el::Level::LEVEL, filename, ( ENTRY ).context.line, func );                           \
        writer.construct( 1, "librealsense" );                                                                         \
        writer << __VA_ARGS__;                                                                                         \
        writer << " [DDS]";                                                                                            \
        if( ( ENTRY ).context.category )                                                                               \
            writer << "[" << ( ENTRY ).context.category << "]";                                                        \
    }                                                                                                                  \
    while( false )


struct log_consumer : eprosima::fastdds::dds::LogConsumer
{
    virtual void Consume( const eprosima::fastdds::dds::Log::Entry & e ) override
    {
        using eprosima::fastdds::dds::Log;
        switch( e.kind )
        {
        case Log::Kind::Error:
            LOG_ENTRY( e, Error, e.message );
            break;
        case Log::Kind::Warning:
            LOG_ENTRY( e, Warning, e.message );
            break;
        case Log::Kind::Info:
            LOG_ENTRY( e, Info, e.message );
            break;
        }
    }
};


PYBIND11_MODULE(NAME, m) {
    m.doc() = R"pbdoc(
        RealSense DDS Server Python Bindings
    )pbdoc";
    m.attr( "__version__" ) = "0.1";  // RS2_API_VERSION_STR;

    // Configure the same logger as librealsense, and default to only errors by default...
    el::Configurations defaultConf;
    defaultConf.setToDefault();
    defaultConf.setGlobally( el::ConfigurationType::ToStandardOutput, "false" );
    defaultConf.set( el::Level::Error, el::ConfigurationType::ToStandardOutput, "true" );
    defaultConf.setGlobally( el::ConfigurationType::Format, "-%levshort- %datetime{%H:%m:%s.%g} %msg (%fbase:%line [%thread])" );
    el::Loggers::reconfigureLogger( "librealsense", defaultConf );
    // And set the DDS logger similarly
    std::unique_ptr< eprosima::fastdds::dds::LogConsumer > consumer( new log_consumer() );
    eprosima::fastdds::dds::Log::ClearConsumers();
    eprosima::fastdds::dds::Log::RegisterConsumer( std::move( consumer ) );
    eprosima::fastdds::dds::Log::SetVerbosity( eprosima::fastdds::dds::Log::Error );

    m.def( "debug", []( bool enable, std::string const & nested ) {
        if( enable )
        {
            //eprosima::fastdds::dds::Log::SetVerbosity( eprosima::fastdds::dds::Log::Info );
        }
        else
        {
            //eprosima::fastdds::dds::Log::SetVerbosity( eprosima::fastdds::dds::Log::Error );
        }
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
    } );

    using librealsense::dds::dds_participant;
    using eprosima::fastrtps::types::ReturnCode_t;
    
    py::class_< dds_participant::listener,
                std::shared_ptr< dds_participant::listener >  // handled with a shared_ptr
                >
        listener( m, "participant.listener" );
    listener  // no ctor: use participant.create_listener()
        .def( FN_FWD( dds_participant::listener,
                      on_writer_added,
                      ( std::string const & guid, std::string const & topic_name ),
                      ( librealsense::dds::dds_guid guid, char const * topic_name ),
                      callback( to_string( guid ), topic_name ); ) )
        .def( FN_FWD( dds_participant::listener,
                      on_writer_removed,
                      ( std::string const & guid, std::string const & topic_name ),
                      ( librealsense::dds::dds_guid guid, char const * topic_name ),
                      callback( to_string( guid ), topic_name ); ) )
        .def( FN_FWD( dds_participant::listener,
                      on_reader_added,
                      ( std::string const & guid, std::string const & topic_name ),
                      ( librealsense::dds::dds_guid guid, char const * topic_name ),
                      callback( to_string( guid ), topic_name ); ) )
        .def( FN_FWD( dds_participant::listener,
                      on_reader_removed,
                      ( std::string const & guid, std::string const & topic_name ),
                      ( librealsense::dds::dds_guid guid, char const * topic_name ),
                      callback( to_string( guid ), topic_name ); ) )
        .def( FN_FWD( dds_participant::listener,
                      on_participant_added,
                      ( std::string const & guid, std::string const & name ),
                      ( librealsense::dds::dds_guid guid, char const * name ),
                      callback( to_string( guid ), name ); ) )
        .def( FN_FWD( dds_participant::listener,
                      on_participant_removed,
                      ( std::string const & guid, std::string const & name ),
                      ( librealsense::dds::dds_guid guid, char const * name ),
                      callback( to_string( guid ), name ); ) )
        .def( FN_FWD( dds_participant::listener,
                      on_type_discovery,
                      ( std::string const & topic_name, std::string const & type_name ),
                      ( char const * topic_name, eprosima::fastrtps::types::DynamicType_ptr dyn_type ),
                      callback( topic_name, dyn_type->get_name() ); ) );

    py::class_< dds_participant,
                std::shared_ptr< dds_participant >  // handled with a shared_ptr
                >
        participant( m, "participant" );
    participant.def( py::init<>() )
        .def( "init", &dds_participant::init, "domain-id"_a, "participant-name"_a )
        .def( "is_valid", &dds_participant::is_valid )
        .def( "__nonzero__", &dds_participant::is_valid ) // Called to implement truth value testing in Python 2
        .def( "__bool__", &dds_participant::is_valid )    // Called to implement truth value testing in Python 3
        .def( "__repr__",
              []( const dds_participant & self ) {
                  std::ostringstream os;
                  os << "<" SNAME ".dds_participant";
                  if( ! self.is_valid() )
                  {
                      os << " NULL";
                  }
                  else
                  {
                      eprosima::fastdds::dds::DomainParticipantQos qos;
                      if( ReturnCode_t::RETCODE_OK == self.get()->get_qos( qos ) )
                          os << " \"" << qos.name() << "\"";
                  }
                  os << ">";
                  return os.str();
            } )
        .def( "create_listener", []( dds_participant & self ) { return self.create_listener(); } );

}
