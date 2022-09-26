/* License: Apache 2.0. See LICENSE file in root directory.
Copyright(c) 2022 Intel Corporation. All Rights Reserved. */

#include "python.hpp"

#include <librealsense2/dds/dds-participant.h>
#include <librealsense2/dds/topics/dds-topics.h>
#include <librealsense2/dds/topics/device-info/deviceInfoPubSubTypes.h>
#include <librealsense2/dds/dds-device-broadcaster.h>
#include <librealsense2/dds/dds-device-watcher.h>
#include <librealsense2/dds/dds-device.h>
#include <librealsense2/dds/dds-guid.h>
#include <librealsense2/dds/dds-topic.h>
#include <librealsense2/dds/dds-topic-reader.h>
#include <librealsense2/dds/dds-utilities.h>

#include <librealsense2/utilities/easylogging/easyloggingpp.h>
#include <fastdds/dds/log/Log.hpp>
#include <fastdds/dds/domain/qos/DomainParticipantQos.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/core/status/SubscriptionMatchedStatus.hpp>
#include <fastrtps/types/DynamicType.h>


#ifndef BUILD_SHARED_LIBS  // shared-init takes care of the else
INITIALIZE_EASYLOGGINGPP
#endif


namespace {
std::string to_string( librealsense::dds::dds_guid const & guid )
{
    std::ostringstream os;
    os << librealsense::dds::print( guid );
    return os.str();
}
}  // namespace


// "When calling a C++ function from Python, the GIL is always held"
// -- since we're not being called from Python but instead are calling it,
// we need to acquire it to not have issues with other threads...
#define FN_FWD_CALL( CLS, FN_NAME, CODE )                                                                              \
    try                                                                                                                \
    {                                                                                                                  \
        py::gil_scoped_acquire gil;                                                                                    \
        CODE                                                                                                           \
    }                                                                                                                  \
    catch( std::exception const & e )                                                                                  \
    {                                                                                                                  \
        std::cerr << "?!?!?!!? exception in python " #CLS "." #FN_NAME " ?!?!?!?!?" << std::endl;                      \
        std::cerr << e.what() << std::endl;                                                                            \
    }                                                                                                                  \
    catch( ... )                                                                                                       \
    {                                                                                                                  \
        std::cerr << "?!?!?!!? exception in python " #CLS "." #FN_NAME " ?!?!?!?!?" << std::endl;                      \
    }
#define FN_FWD( CLS, FN_NAME, PY_ARGS, FN_ARGS, CODE )                                                                 \
    #FN_NAME, []( CLS & self, std::function < void PY_ARGS > callback ) {                                              \
        self.FN_NAME( [callback] FN_ARGS { FN_FWD_CALL( CLS, FN_NAME, CODE ); } );                                     \
    }


// Convert FastDDS Log::Entry to EasyLogging log (see ELPP_WRITE_LOG)
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
        .def( "__nonzero__", &dds_participant::is_valid )  // Called to implement truth value testing in Python 2
        .def( "__bool__", &dds_participant::is_valid )     // Called to implement truth value testing in Python 3
        .def( "__repr__",
              []( const dds_participant & self ) {
                  std::ostringstream os;
                  os << "<" SNAME ".participant";
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

    // The 'types' submodule will contain all the known topic types in librs
    // These are used alongside the 'dds_topic' class:
    //      topic = dds.topic( p, dds.types.device_info(), "realsense/device-info" )
    auto types = m.def_submodule( "types", "all the types that " SNAME " can work with" );

    // We need to declare a basic TypeSupport or else dds_topic ctor won't work
    using eprosima::fastdds::dds::TypeSupport;
    py::class_< TypeSupport > topic_type( types, "topic_type" );

    // Another submodule, 'raw' is used with dds_topic to actually send and receive messages, e.g.:
    //      reader = dds.topic_reader( topic )
    //      ...
    //      info = dds.device_info( dds.raw.device_info( reader ) )
    //auto raw = m.def_submodule( "raw", "all the topics that " SNAME " can work with" );


    using librealsense::dds::dds_topic;
    py::class_< dds_topic, std::shared_ptr< dds_topic > >( m, "topic" )
        .def( py::init< std::shared_ptr< dds_participant > const &,
                        eprosima::fastdds::dds::TypeSupport const &,
                        char const * >() )
        .def( "get_participant", &dds_topic::get_participant )
        .def( "__repr__", []( dds_topic const & self ) {
            std::ostringstream os;
            os << "<" SNAME ".topic \"" << self->get_name() << "\"";
            os << ">";
            return os.str();
        } );

    using reader_qos = librealsense::dds::dds_topic_reader::reader_qos;
    py::class_< reader_qos >( m, "reader_qos" )
        .def( "__repr__", []( dds_topic const & self ) {
            std::ostringstream os;
            os << "<" SNAME ".reader_qos";
            os << ">";
            return os.str();
        } );

    using librealsense::dds::dds_topic_reader;
    py::class_< dds_topic_reader, std::shared_ptr< dds_topic_reader > >( m, "topic_reader" )
        .def( py::init< std::shared_ptr< dds_topic > const & >() )
        .def( FN_FWD( dds_topic_reader, on_data_available, (), (), callback(); ) )
        .def( FN_FWD( dds_topic_reader,
                      on_subscription_matched,
                      (),
                      (eprosima::fastdds::dds::SubscriptionMatchedStatus const &),
                      callback(); ) )
        .def( "run", &dds_topic_reader::run )
        .def( "qos", []() { return reader_qos(); } );


    // The actual types are declared as functions and not classes: the py::init<> inheritance rules are pretty strict
    // and, coupled with shared_ptr usage, are very hard to get around. This is much simpler...
    using librealsense::dds::topics::device_info;
    using raw_device_info = librealsense::dds::topics::raw::device_info;
    types.def( "device_info", []() { return TypeSupport( new device_info::type ); } );

    py::class_< device_info >( m, "device_info" )
        .def( py::init<>() )
        .def_readwrite( "name", &device_info::name )
        .def_readwrite( "serial", &device_info::serial )
        .def_readwrite( "product_line", &device_info::product_line )
        .def_readwrite( "locked", &device_info::locked )
        .def_readwrite( "topic_root", &device_info::topic_root )
        .def( "invalidate", &device_info::invalidate )
        .def( "is_valid", &device_info::is_valid )
        .def( "__nonzero__", &device_info::is_valid )  // Called to implement truth value testing in Python 2
        .def( "__bool__", &device_info::is_valid )     // Called to implement truth value testing in Python 3
        .def( "__repr__",
              []( device_info const & self ) {
                  std::ostringstream os;
                  os << "<" SNAME ".device_info ";
                  if( ! self.is_valid() )
                      os << "INVALID";
                  else
                  {
                      if( !self.name.empty() )
                          os << "\"" << self.name << "\" ";
                      if( !self.serial.empty() )
                          os << "s/n \"" << self.serial << "\" ";
                      if( !self.topic_root.empty() )
                          os << "topic-root \"" << self.topic_root << "\" ";
                      if( !self.product_line.empty() )
                          os << "product-line \"" << self.product_line << "\" ";
                      os << ( self.locked ? "locked" : "unlocked" );
                  }
                  os << ">";
                  return os.str();
              } )
        .def( "take_next",
              []( dds_topic_reader & reader ) {
                  auto actual_type = reader.topic()->get()->get_type_name();
                  if( actual_type != device_info::type().getName() )
                      throw std::runtime_error( "can't initialize raw::device_info from " + actual_type );
                  device_info data;
                  if( ! device_info::take_next( reader, &data ) )
                      assert( ! data.is_valid() );
                  return data;
              } )
        .def( "create_topic", &device_info::create_topic )
        .attr( "TOPIC_NAME" ) = device_info::TOPIC_NAME;

    //py::class_< raw_device_info, std::shared_ptr< raw_device_info > >( raw, "device_info" )
    //    .def( py::init<>() )
    //    .def( py::init( []( dds_topic_reader const & reader ) {
    //        auto actual_type = reader.topic()->get()->get_type_name();
    //        if( actual_type != "librealsense::dds::topics::raw::device_info" )
    //            throw std::runtime_error( "can't initialize raw::device_info from " + actual_type );
    //        raw_device_info raw_data;
    //        eprosima::fastdds::dds::SampleInfo info;
    //        DDS_API_CALL( reader->take_next_sample( &raw_data, &info ) );
    //        if( ! info.valid_data )
    //            throw std::runtime_error( "invalid data" );
    //        return raw_data;
    //    } ) );


    using librealsense::dds::dds_device_broadcaster;
    py::class_< dds_device_broadcaster >( m, "device_broadcaster" )
        .def( py::init< std::shared_ptr< dds_participant > const & >() )
        .def( "run", &dds_device_broadcaster::run )
        .def( "add_device", &dds_device_broadcaster::add_device )
        .def( "remove_device", &dds_device_broadcaster::remove_device );

    // same as in pyrs_internal.cpp
    py::class_< rs2_video_stream > video_stream( m, "video_stream" );
    video_stream.def( py::init<>() )
        .def_readwrite( "type", &rs2_video_stream::type )
        .def_readwrite( "index", &rs2_video_stream::index )
        .def_readwrite( "uid", &rs2_video_stream::uid )
        .def_readwrite( "width", &rs2_video_stream::width )
        .def_readwrite( "height", &rs2_video_stream::height )
        .def_readwrite( "fps", &rs2_video_stream::fps )
        .def_readwrite( "bpp", &rs2_video_stream::bpp )
        .def_readwrite( "fmt", &rs2_video_stream::fmt )
        .def_readwrite( "intrinsics", &rs2_video_stream::intrinsics );

    // same as in pyrs_internal.cpp
    py::class_< rs2_motion_stream > motion_stream( m, "motion_stream" );
    motion_stream.def( py::init<>() )
        .def_readwrite( "type", &rs2_motion_stream::type )
        .def_readwrite( "index", &rs2_motion_stream::index )
        .def_readwrite( "uid", &rs2_motion_stream::uid )
        .def_readwrite( "fps", &rs2_motion_stream::fps )
        .def_readwrite( "fmt", &rs2_motion_stream::fmt )
        .def_readwrite( "intrinsics", &rs2_motion_stream::intrinsics );

    using librealsense::dds::dds_device;
    py::class_< dds_device,
                std::shared_ptr< dds_device >  // handled with a shared_ptr
                >( m, "device" )
        .def( "device_info", &dds_device::device_info )
        .def( "guid", []( dds_device const & self ) { return to_string( self.guid() ); } )
        .def( "is_running", &dds_device::is_running )
        .def( "run", &dds_device::run )
        .def( "num_of_sensors", &dds_device::num_of_sensors )
        .def( FN_FWD( dds_device,
            foreach_sensor,
            ( std::string const & ),
            ( std::string const & name ),
            callback( name ); ) )
        .def( "foreach_video_profile",
              []( dds_device const & self,
                  size_t sensor_index,
                  std::function< void( rs2_video_stream const & profile, bool is_default ) > callback ) {
                  self.foreach_video_profile(
                      sensor_index,
                      [callback]( rs2_video_stream const & profile, bool is_default ) {
                          FN_FWD_CALL( dds_device, foreach_video_profile, callback( profile, is_default ); );
                      } );
              }, py::call_guard< py::gil_scoped_release >() )
        .def( "foreach_motion_profile",
              []( dds_device const & self,
                  size_t sensor_index,
                  std::function< void( rs2_motion_stream const & profile, bool is_default ) > callback ) {
                  self.foreach_motion_profile(
                      sensor_index,
                      [callback]( rs2_motion_stream const & profile, bool is_default ) {
                          FN_FWD_CALL( dds_device, foreach_motion_profile, callback( profile, is_default ); );
                      } );
              }, py::call_guard< py::gil_scoped_release >() )
        .def( "__repr__", []( dds_device const & self ) {
            std::ostringstream os;
            os << "<" SNAME ".device[";
            os << to_string( self.guid() );
            if( ! self.device_info().name.empty() )
                os << " \"" << self.device_info().name << "\"";
            if( ! self.device_info().serial.empty() )
                os << " s/n \"" << self.device_info().serial << "\"";
            os << "]>";
            return os.str();
        } );

    using librealsense::dds::dds_device_watcher;
    py::class_< dds_device_watcher >( m, "device_watcher" )
        .def( py::init< std::shared_ptr< dds_participant > const & >() )
        .def( "start", &dds_device_watcher::start )
        .def( "stop", &dds_device_watcher::stop )
        .def( "is_stopped", &dds_device_watcher::is_stopped )
        .def( FN_FWD( dds_device_watcher,
                      on_device_added,
                      (std::shared_ptr< dds_device > const &),
                      ( std::shared_ptr< dds_device > const & dev ),
                      callback( dev ); ) )
        .def( FN_FWD( dds_device_watcher,
                      on_device_removed,
                      (std::shared_ptr< dds_device > const &),
                      ( std::shared_ptr< dds_device > const & dev ),
                      callback( dev ); ) )
        .def( "foreach_device",
              []( dds_device_watcher const & self,
                  std::function< bool( std::shared_ptr< dds_device > const & ) > callback ) {
                  self.foreach_device(
                      [callback]( std::shared_ptr< dds_device > const & dev ) { return callback( dev ); } );
              }, py::call_guard< py::gil_scoped_release >() );
}
