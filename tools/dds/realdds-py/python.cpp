/* License: Apache 2.0. See LICENSE file in root directory.
Copyright(c) 2022 Intel Corporation. All Rights Reserved. */

#include "python.hpp"

#include <realdds/dds-participant.h>
#include <realdds/topics/dds-topics.h>
#include <realdds/topics/device-info/deviceInfoPubSubTypes.h>
#include <realdds/dds-device-broadcaster.h>
#include <realdds/dds-device-server.h>
#include <realdds/dds-stream-server.h>
#include <realdds/dds-stream-profile.h>
#include <realdds/dds-device-watcher.h>
#include <realdds/dds-device.h>
#include <realdds/dds-stream.h>
#include <realdds/dds-guid.h>
#include <realdds/dds-topic.h>
#include <realdds/dds-topic-reader.h>
#include <realdds/dds-utilities.h>
#include <realdds/dds-log-consumer.h>

#include <librealsense2/utilities/easylogging/easyloggingpp.h>
#include <fastdds/dds/domain/qos/DomainParticipantQos.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/core/status/SubscriptionMatchedStatus.hpp>
#include <fastrtps/types/DynamicType.h>

#include <third-party/json.hpp>


#ifndef BUILD_SHARED_LIBS  // shared-init takes care of the else
INITIALIZE_EASYLOGGINGPP
#endif


namespace {

std::string to_string( realdds::dds_guid const & guid )
{
    return realdds::print( guid );
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
        LOG_ERROR( "EXCEPTION in python " #CLS "." #FN_NAME ": " << e.what() ); \
    }                                                                                                                  \
    catch( ... )                                                                                                       \
    {                                                                                                                  \
        LOG_ERROR( "UNKNOWN EXCEPTION in python " #CLS "." #FN_NAME ); \
    }
#define FN_FWD( CLS, FN_NAME, PY_ARGS, FN_ARGS, CODE )                                                                 \
    #FN_NAME, []( CLS & self, std::function < void PY_ARGS > callback ) {                                              \
        self.FN_NAME( [&self,callback] FN_ARGS { FN_FWD_CALL( CLS, FN_NAME, CODE ); } );                                     \
    }


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
    eprosima::fastdds::dds::Log::ClearConsumers();
    eprosima::fastdds::dds::Log::RegisterConsumer( realdds::log_consumer::create() );
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

    using realdds::dds_guid;
    py::class_< dds_guid >( m, "guid" )
        .def( py::init<>() )
        .def( "__nonzero__", []( dds_guid const& self ) { return self != dds_guid::unknown(); } )  // Called to implement truth value testing in Python 2
        .def( "__bool__", []( dds_guid const& self ) { return self != dds_guid::unknown(); } )     // Called to implement truth value testing in Python 3
        .def( "__repr__", []( dds_guid const& self ) { return to_string( self ); } );

    using realdds::dds_participant;
    using eprosima::fastrtps::types::ReturnCode_t;
    
    py::class_< dds_participant::listener,
                std::shared_ptr< dds_participant::listener >  // handled with a shared_ptr
                >
        listener( m, "participant.listener" );
    listener  // no ctor: use participant.create_listener()
        .def( FN_FWD( dds_participant::listener,
                      on_writer_added,
                      ( dds_guid guid, char const * topic_name ),
                      ( dds_guid guid, char const * topic_name ),
                      callback( guid, topic_name ); ) )
        .def( FN_FWD( dds_participant::listener,
                      on_writer_removed,
                      ( dds_guid guid, char const * topic_name ),
                      ( dds_guid guid, char const * topic_name ),
                      callback( guid, topic_name ); ) )
        .def( FN_FWD( dds_participant::listener,
                      on_reader_added,
                      ( dds_guid guid, char const * topic_name ),
                      ( dds_guid guid, char const * topic_name ),
                      callback( guid, topic_name ); ) )
        .def( FN_FWD( dds_participant::listener,
                      on_reader_removed,
                      ( dds_guid guid, char const * topic_name ),
                      ( dds_guid guid, char const * topic_name ),
                      callback( guid, topic_name ); ) )
        .def( FN_FWD( dds_participant::listener,
                      on_participant_added,
                      ( dds_guid guid, char const * name ),
                      ( dds_guid guid, char const * name ),
                      callback( guid, name ); ) )
        .def( FN_FWD( dds_participant::listener,
                      on_participant_removed,
                      ( dds_guid guid, char const * name ),
                      ( dds_guid guid, char const * name ),
                      callback( guid, name ); ) )
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
        .def( "guid", &dds_participant::guid )
        .def( "create_guid", &dds_participant::create_guid )
        .def( "__nonzero__", &dds_participant::is_valid )  // Called to implement truth value testing in Python 2
        .def( "__bool__", &dds_participant::is_valid )     // Called to implement truth value testing in Python 3
        .def( "name",
              []( dds_participant const & self ) {
                  eprosima::fastdds::dds::DomainParticipantQos qos;
                  if( ReturnCode_t::RETCODE_OK == self.get()->get_qos( qos ) )
                      return std::string( qos.name() );
                  return std::string();
              } )
        .def( "name_from_guid", []( dds_guid const & guid ) { return dds_participant::name_from_guid( guid ); } )
        .def( "names", []( dds_participant const & self ) { return self.get()->get_participant_names(); } )
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
                      os << " " << to_string( self.guid() );
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


    using realdds::dds_topic;
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

    using reader_qos = realdds::dds_topic_reader::qos;
    py::class_< reader_qos >( m, "reader_qos" )
        .def( "__repr__", []( reader_qos const & self ) {
            std::ostringstream os;
            os << "<" SNAME ".reader_qos";
            os << ">";
            return os.str();
        } );

    using realdds::dds_topic_reader;
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
    using realdds::topics::device_info;
    using raw_device_info = realdds::topics::raw::device_info;
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
                      if( ! self.name.empty() )
                          os << "\"" << self.name << "\" ";
                      if( ! self.serial.empty() )
                          os << "s/n \"" << self.serial << "\" ";
                      if( ! self.topic_root.empty() )
                          os << "@ \"" << self.topic_root << "\" ";
                      if( ! self.product_line.empty() )
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
    //        if( actual_type != "realdds::topics::raw::device_info" )
    //            throw std::runtime_error( "can't initialize raw::device_info from " + actual_type );
    //        raw_device_info raw_data;
    //        eprosima::fastdds::dds::SampleInfo info;
    //        DDS_API_CALL( reader->take_next_sample( &raw_data, &info ) );
    //        if( ! info.valid_data )
    //            throw std::runtime_error( "invalid data" );
    //        return raw_data;
    //    } ) );


    using realdds::dds_device_broadcaster;
    py::class_< dds_device_broadcaster >( m, "device_broadcaster" )
        .def( py::init< std::shared_ptr< dds_participant > const & >() )
        .def( "run", &dds_device_broadcaster::run )
        .def( "add_device", &dds_device_broadcaster::add_device )
        .def( "remove_device", &dds_device_broadcaster::remove_device );

    using realdds::dds_stream_format;
    py::class_< dds_stream_format >( m, "stream_format" )
        .def( py::init<>() )
        .def( py::init< std::string const & >() )
        .def( "is_valid", &dds_stream_format::is_valid )
        .def( "to_rs2", &dds_stream_format::to_rs2 )
        .def( "__nonzero__", &dds_stream_format::is_valid )  // Called to implement truth value testing in Python 2
        .def( "__bool__", &dds_stream_format::is_valid )     // Called to implement truth value testing in Python 3
        .def( "__repr__", []( dds_stream_format const & self ) { return self.to_string(); } );

    using realdds::dds_stream_uid;
    py::class_< dds_stream_uid >( m, "stream_uid" )
        .def( py::init<>() )
        .def( py::init< uint32_t >() )
        .def( py::init< int, int >() )
        .def_readwrite( "whole", &dds_stream_uid::whole )
        .def_readwrite( "sid", &dds_stream_uid::sid )
        .def_readwrite( "index", &dds_stream_uid::index )
        .def( "to_string", &dds_stream_uid::to_string )
        .def( "__repr__", &dds_stream_uid::to_string );

    using realdds::dds_stream_profile;
    py::class_< dds_stream_profile, std::shared_ptr< dds_stream_profile > > stream_profile_base( m, "stream_profile" );
    stream_profile_base
        .def( "uid", &dds_stream_profile::uid )
        .def( "frequency", &dds_stream_profile::frequency )
        .def( "format", &dds_stream_profile::format )
        .def( "stream", &dds_stream_profile::stream )
        .def( "to_string", &dds_stream_profile::to_string )
        .def( "details_to_string", &dds_stream_profile::details_to_string )
        .def( "to_json", []( dds_stream_profile const & self ) { return self.to_json().dump(); } )
        .def( "__repr__", []( dds_stream_profile const & self ) {
            std::ostringstream os;
            std::string self_as_string = self.to_string();  // <video 0xUID ...>
            std::string type;
            auto stream = self.stream();
            if( stream )
                type = std::string( stream->type_string() ) + '_';
            os << "<" SNAME "." << type << "stream_profile " << ( self_as_string.c_str() + 1 );
            return os.str();
        } );

    using realdds::dds_video_stream_profile;
    py::class_< dds_video_stream_profile, std::shared_ptr< dds_video_stream_profile > >( m, "video_stream_profile", stream_profile_base )
        .def( py::init< dds_stream_uid, dds_stream_format, int16_t, uint16_t, uint16_t, uint8_t >() )
        .def( "width", &dds_video_stream_profile::width )
        .def( "height", &dds_video_stream_profile::height )
        .def( "bytes_per_pixel", &dds_video_stream_profile::bytes_per_pixel );

    using realdds::dds_motion_stream_profile;
    py::class_< dds_motion_stream_profile, std::shared_ptr< dds_motion_stream_profile > >( m, "motion_stream_profile", stream_profile_base )
        .def( py::init< dds_stream_uid, dds_stream_format, int16_t >() );

    using realdds::dds_stream_base;
    py::class_< dds_stream_base, std::shared_ptr< dds_stream_base > > stream_base( m, "stream_base" );
    stream_base
        .def( "name", &dds_stream_base::name )
        .def( "sensor_name", &dds_stream_base::sensor_name )
        .def( "type_string", &dds_stream_base::type_string )
        .def( "profiles", &dds_stream_base::profiles )
        .def( "init_profiles", &dds_stream_base::init_profiles )
        .def( "default_profile_index", &dds_stream_base::default_profile_index )
        .def( "is_open", &dds_stream_base::is_open )
        .def( "is_streaming", &dds_stream_base::is_streaming )
        .def( "get_topic", &dds_stream_base::get_topic );

    using realdds::dds_stream_server;
    py::class_< dds_stream_server, std::shared_ptr< dds_stream_server > > stream_server_base( m, "stream_server", stream_base );
    stream_server_base  //
        .def( "__repr__", []( dds_stream_server const & self ) {
            std::ostringstream os;
            os << "<" SNAME "." << self.type_string() << "_stream_server \"" << self.name() << "\" [";
            for( auto & p : self.profiles() )
                os << p->to_string();
            os << ']';
            os << ' ' << self.default_profile_index();
            os << '>';
            return os.str();
        } );

    using realdds::dds_video_stream_server;
    py::class_< dds_video_stream_server, std::shared_ptr< dds_video_stream_server > >( m, "video_stream_server", stream_server_base )
        .def( py::init< std::string const &, std::string const & >(), "stream_name"_a, "sensor_name"_a );

    using realdds::dds_motion_stream_server;
    py::class_< dds_motion_stream_server, std::shared_ptr< dds_motion_stream_server > >( m, "motion_stream_server", stream_server_base )
        .def( py::init< std::string const &, std::string const & >(), "stream_name"_a, "sensor_name"_a );

    using realdds::dds_device_server;
    py::class_< dds_device_server >( m, "device_server" )
        .def( py::init< std::shared_ptr< dds_participant > const&, std::string const& >() )
        .def( "init", &dds_device_server::init );

    using realdds::dds_stream;
    py::class_< dds_stream, std::shared_ptr< dds_stream > > stream_client_base( m, "stream", stream_base );

    using realdds::dds_video_stream;
    py::class_< dds_video_stream, std::shared_ptr< dds_video_stream > >( m, "video_stream", stream_client_base )
        .def( py::init< std::string const &, std::string const & >(), "stream_name"_a, "sensor_name"_a )
        .def( "__repr__", []( dds_video_stream const & self ) {
            std::ostringstream os;
            os << "<" SNAME ".video_stream \"" << self.name() << "\" [";
            for( auto & p : self.profiles() )
                os << p->to_string();
            os << ']';
            os << ' ' << self.default_profile_index();
            os << '>';
            return os.str();
        } );

    using realdds::dds_motion_stream;
    py::class_< dds_motion_stream, std::shared_ptr< dds_motion_stream > >( m, "motion_stream", stream_client_base )
        .def( py::init< std::string const &, std::string const & >(), "stream_name"_a, "sensor_name"_a )
        .def( "__repr__", []( dds_motion_stream const & self ) {
            std::ostringstream os;
            os << "<" SNAME ".motion_stream \"" << self.name() << "\" [";
            for( auto & p : self.profiles() )
                os << p->to_string();
            os << ']';
            os << ' ' << self.default_profile_index();
            os << '>';
            return os.str();
        } );

    using realdds::dds_device;
    py::class_< dds_device,
                std::shared_ptr< dds_device >  // handled with a shared_ptr
                >( m, "device" )
        .def( py::init( &dds_device::create ))
        .def( "device_info", &dds_device::device_info )
        .def( "participant", &dds_device::participant )
        .def( "guid", &dds_device::guid )
        .def( "is_running", &dds_device::is_running )
        .def( "run", &dds_device::run, py::call_guard< py::gil_scoped_release >() )
        .def( "n_streams", &dds_device::number_of_streams )
        .def( "streams",
              []( dds_device const & self ) {
                  std::vector< std::shared_ptr< dds_stream > > streams;
                  self.foreach_stream(
                      [&]( std::shared_ptr< dds_stream > const & stream ) { streams.push_back( stream ); } );
                  return streams;
              } )
        .def( "__repr__", []( dds_device const & self ) {
            std::ostringstream os;
            os << "<" SNAME ".device ";
            os << self.participant()->print( self.guid() );
            if( ! self.device_info().name.empty() )
                os << " \"" << self.device_info().name << "\"";
            if( ! self.device_info().serial.empty() )
                os << " s/n \"" << self.device_info().serial << "\"";
            os << ">";
            return os.str();
        } );

    using realdds::dds_device_watcher;
    py::class_< dds_device_watcher >( m, "device_watcher" )
        .def( py::init< std::shared_ptr< dds_participant > const & >() )
        .def( "start", &dds_device_watcher::start )
        .def( "stop", &dds_device_watcher::stop )
        .def( "is_stopped", &dds_device_watcher::is_stopped )
        .def( FN_FWD( dds_device_watcher,
                      on_device_added,
                      ( dds_device_watcher const &, std::shared_ptr< dds_device > const & ),
                      ( std::shared_ptr< dds_device > const & dev ),
                      callback( self, dev ); ) )
        .def( FN_FWD( dds_device_watcher,
                      on_device_removed,
                      ( dds_device_watcher const &, std::shared_ptr< dds_device > const & ),
                      ( std::shared_ptr< dds_device > const & dev ),
                      callback( self, dev ); ) )
        .def( "foreach_device",
              []( dds_device_watcher const & self,
                  std::function< bool( std::shared_ptr< dds_device > const & ) > callback ) {
                  self.foreach_device(
                      [callback]( std::shared_ptr< dds_device > const & dev ) { return callback( dev ); } );
              }, py::call_guard< py::gil_scoped_release >() );
}
