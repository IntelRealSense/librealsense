// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <realdds/dds-defines.h>
#include <realdds/dds-participant.h>
#include <realdds/topics/flexible/flexible-msg.h>
#include <realdds/topics/flexible/flexiblePubSubTypes.h>
#include <realdds/topics/image/image-msg.h>
#include <realdds/topics/ros2/ros2imagePubSubTypes.h>
#include <realdds/topics/dds-topic-names.h>
#include <realdds/dds-device-broadcaster.h>
#include <realdds/dds-device-server.h>
#include <realdds/dds-stream-server.h>
#include <realdds/dds-stream-profile.h>
#include <realdds/dds-device-watcher.h>
#include <realdds/dds-device.h>
#include <realdds/dds-stream.h>
#include <realdds/dds-guid.h>
#include <realdds/dds-time.h>
#include <realdds/dds-topic.h>
#include <realdds/dds-topic-reader.h>
#include <realdds/dds-topic-writer.h>
#include <realdds/dds-publisher.h>
#include <realdds/dds-subscriber.h>
#include <realdds/dds-log-consumer.h>
#include <realdds/dds-stream-sensor-bridge.h>
#include <realdds/dds-metadata-syncer.h>

#include <rsutils/easylogging/easyloggingpp.h>

#include <fastdds/dds/domain/qos/DomainParticipantQos.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <fastrtps/types/DynamicType.h>

#include <rsutils/py/pybind11.h>


#define NAME pyrealdds
#define SNAME "pyrealdds"


namespace {

std::string to_string( realdds::dds_guid const & guid )
{
    return realdds::print( guid );
}

}  // namespace


PYBIND11_MODULE(NAME, m) {
    m.doc() = R"pbdoc(
        RealSense DDS Server Python Bindings
    )pbdoc";
    m.attr( "__version__" ) = "0.1";  // RS2_API_VERSION_STR;

    // Configure the same logger as librealsense, and default to only errors by default...
    rsutils::configure_elpp_logger();
    // And set the DDS logger similarly
    eprosima::fastdds::dds::Log::ClearConsumers();
    eprosima::fastdds::dds::Log::RegisterConsumer( realdds::log_consumer::create() );
    eprosima::fastdds::dds::Log::SetVerbosity( eprosima::fastdds::dds::Log::Error );

    m.def( "debug",
           &rsutils::configure_elpp_logger,
           py::arg( "enable" ),
           py::arg( "nested-string" ) = "",
           py::arg( "logger" ) = LIBREALSENSE_ELPP_ID );

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

    // The 'topics' submodule will contain pointers to the topic names:
    auto topics = m.def_submodule( "topics", "all the topics that " SNAME " knows" );
    topics.attr( "device_info" ) = realdds::topics::DEVICE_INFO_TOPIC_NAME;

    // We need to declare a basic TypeSupport or else dds_topic ctor won't work
    using eprosima::fastdds::dds::TypeSupport;
    py::class_< TypeSupport > topic_type( types, "topic_type" );

    // Another submodule, 'raw' is used with dds_topic to actually send and receive messages, e.g.:
    //      reader = dds.topic_reader( topic )
    //      ...
    //      info = dds.device_info( dds.raw.device_info( reader ) )
    //auto raw = m.def_submodule( "raw", "all the topics that " SNAME " can work with" );

    using realdds::video_intrinsics;
    py::class_< video_intrinsics, std::shared_ptr< video_intrinsics > >( m, "video_intrinsics" )
        .def( py::init<>() )
        .def_readwrite( "width", &video_intrinsics::width )
        .def_readwrite( "height", &video_intrinsics::height )
        .def_readwrite( "principal_point_x", &video_intrinsics::principal_point_x )
        .def_readwrite( "principal_point_y", &video_intrinsics::principal_point_y )
        .def_readwrite( "focal_lenght_x", &video_intrinsics::focal_lenght_x )
        .def_readwrite( "focal_lenght_y", &video_intrinsics::focal_lenght_y )
        .def_readwrite( "distortion_model", &video_intrinsics::distortion_model )
        .def_readwrite( "distortion_coeffs", &video_intrinsics::distortion_coeffs );

    using realdds::motion_intrinsics;
    py::class_< motion_intrinsics, std::shared_ptr< motion_intrinsics > >( m, "motion_intrinsics" )
        .def( py::init<>() )
        .def_readwrite( "data", &motion_intrinsics::data )
        .def_readwrite( "noise_variances", &motion_intrinsics::noise_variances )
        .def_readwrite( "bias_variances", &motion_intrinsics::bias_variances );

    using realdds::extrinsics;
    py::class_< extrinsics, std::shared_ptr< extrinsics > >( m, "extrinsics" )
        .def( py::init<>() )
        .def_readwrite( "rotation", &extrinsics::rotation )
        .def_readwrite( "translation", &extrinsics::translation );

    using realdds::dds_topic;
    py::class_< dds_topic, std::shared_ptr< dds_topic > >( m, "topic" )
        .def( py::init< std::shared_ptr< dds_participant > const &,
                        eprosima::fastdds::dds::TypeSupport const &,
                        char const * >() )
        .def( "get_participant", &dds_topic::get_participant )
        .def( "name", []( dds_topic const & self ) { return self->get_name(); } )
        .def( "__repr__", []( dds_topic const & self ) {
            std::ostringstream os;
            os << "<" SNAME ".topic \"" << self->get_name() << "\"";
            os << ">";
            return os.str();
        } );

    using durability = eprosima::fastdds::dds::DurabilityQosPolicyKind;
    py::enum_< durability >( m, "durability" )
        .value( "volatile", eprosima::fastdds::dds::DurabilityQosPolicyKind::VOLATILE_DURABILITY_QOS )
        .value( "transient_local", eprosima::fastdds::dds::DurabilityQosPolicyKind::TRANSIENT_LOCAL_DURABILITY_QOS )
        .value( "transient", eprosima::fastdds::dds::DurabilityQosPolicyKind::TRANSIENT_DURABILITY_QOS );
    using reliability = eprosima::fastdds::dds::ReliabilityQosPolicyKind;
    py::enum_< reliability >( m, "reliability" )
        .value( "reliable", eprosima::fastdds::dds::ReliabilityQosPolicyKind::RELIABLE_RELIABILITY_QOS )
        .value( "best_effort", eprosima::fastdds::dds::ReliabilityQosPolicyKind::BEST_EFFORT_RELIABILITY_QOS );

    using reader_qos = realdds::dds_topic_reader::qos;
    py::class_< reader_qos >( m, "reader_qos" )  //
        .def( "__repr__", []( reader_qos const & self ) {
            std::ostringstream os;
            os << "<" SNAME ".reader_qos";
            os << ">";
            return os.str();
        } );

    using realdds::dds_topic_reader;
    py::class_< dds_topic_reader, std::shared_ptr< dds_topic_reader > >( m, "topic_reader" )
        .def( py::init< std::shared_ptr< dds_topic > const & >() )
        .def( FN_FWD( dds_topic_reader, on_data_available, (dds_topic_reader &), (), callback( self ); ) )
        .def( FN_FWD( dds_topic_reader,
                      on_subscription_matched,
                      (dds_topic_reader &, int),
                      ( eprosima::fastdds::dds::SubscriptionMatchedStatus const & status ),
                      callback( self, status.current_count_change ); ) )
        .def( "topic", &dds_topic_reader::topic )
        .def( "run", &dds_topic_reader::run )
        .def( "qos", []() { return reader_qos(); } )
        .def( "qos", []( reliability r, durability d ) { return reader_qos( r, d ); } );

    using writer_qos = realdds::dds_topic_writer::qos;
    py::class_< writer_qos >( m, "writer_qos" )  //
        .def( "__repr__", []( writer_qos const & self ) {
            std::ostringstream os;
            os << "<" SNAME ".writer_qos";
            os << ">";
            return os.str();
        } );

    using realdds::dds_publisher;
    py::class_< dds_publisher, std::shared_ptr< dds_publisher > >( m, "publisher" )
        .def( py::init< std::shared_ptr< dds_participant > const & >() );

    using realdds::dds_subscriber;
    py::class_< dds_subscriber, std::shared_ptr< dds_subscriber > >( m, "subscriber" )
        .def( py::init< std::shared_ptr< dds_participant > const & >() );

    using realdds::dds_topic_writer;
    py::class_< dds_topic_writer, std::shared_ptr< dds_topic_writer > >( m, "topic_writer" )
        .def( py::init< std::shared_ptr< dds_topic > const & >() )
        .def( FN_FWD( dds_topic_writer,
                      on_publication_matched,
                      (dds_topic_writer &, int),
                      ( eprosima::fastdds::dds::PublicationMatchedStatus const & status ),
                      callback( self, status.current_count_change ); ) )
        .def( "topic", &dds_topic_writer::topic )
        .def( "run", &dds_topic_writer::run )
        .def( "qos", []() { return writer_qos(); } )
        .def( "qos", []( reliability r, durability d ) { return writer_qos( r, d ); } );


    // The actual types are declared as functions and not classes: the py::init<> inheritance rules are pretty strict
    // and, coupled with shared_ptr usage, are very hard to get around. This is much simpler...
    using realdds::topics::device_info;
    using realdds::topics::flexible_msg;
    types.def( "device_info", []() { return TypeSupport( new flexible_msg::type ); } );

    py::class_< device_info >( m, "device_info" )
        .def( py::init<>() )
        .def_readwrite( "name", &device_info::name )
        .def_readwrite( "serial", &device_info::serial )
        .def_readwrite( "product_line", &device_info::product_line )
        .def_readwrite( "product_id", &device_info::product_id )
        .def_readwrite( "locked", &device_info::locked )
        .def_readwrite( "topic_root", &device_info::topic_root )
        .def_static( "from_json", &device_info::from_json )
        .def( "to_json", &device_info::to_json )
        .def( "__repr__",
              []( device_info const & self ) {
                  std::ostringstream os;
                  os << "<" SNAME ".device_info ";
                    if( ! self.name.empty() )
                        os << "\"" << self.name << "\" ";
                    if( ! self.serial.empty() )
                        os << "s/n \"" << self.serial << "\" ";
                    if( ! self.topic_root.empty() )
                        os << "@ \"" << self.topic_root << "\" ";
                    if( ! self.product_line.empty() )
                        os << "product-line \"" << self.product_line << "\" ";
                    if( !self.product_id.empty() )
                        os << "product-id \"" << self.product_id << "\" ";
                    os << ( self.locked ? "locked" : "unlocked" );
                  os << ">";
                  return os.str();
              } );

    using realdds::topics::flexible_msg;
    using raw_flexible = realdds::topics::raw::flexible;
    types.def( "flexible", []() { return TypeSupport( new flexible_msg::type ); } );

    py::enum_< flexible_msg::data_format >( m, "flexible.data_format" )
        .value( "json", flexible_msg::data_format::JSON )
        .value( "cbor", flexible_msg::data_format::CBOR )
        .value( "custom", flexible_msg::data_format::CUSTOM );

    using eprosima::fastdds::dds::SampleInfo;
    py::class_< SampleInfo >( m, "sample_info" )  //
        .def( py::init<>() )
        .def( "source_timestamp", []( SampleInfo const & self ) { return self.source_timestamp.to_ns(); } )
        .def( "reception_timestamp", []( SampleInfo const & self ) { return self.reception_timestamp.to_ns(); } );


    using realdds::dds_time;
    using realdds::dds_nsec;
    py::class_< dds_time >( m, "time" )
        .def( py::init<>() )
        .def_readwrite( "seconds", &dds_time::seconds )
        .def_readwrite( "nanosec", &dds_time::nanosec )
        .def( "to_ns", &dds_time::to_ns )
        .def_static( "from_ns", []( dds_nsec ns ) { return realdds::time_from( ns ); } )
        .def_static( "from_double", []( long double d ) { return realdds::dds_time( d ); } )
        .def( "to_double", &realdds::time_to_double )
        .def( "__repr__", &realdds::time_to_string );


    // We need a timestamp function that returns timestamps in the same domain as the sample-info timestamps
    using realdds::timestr;
    m.def( "now", []() { return realdds::now(); } );

    py::enum_< timestr::no_suffix_t >( m, "no_suffix_t" );
    m.attr( "no_suffix" ) = timestr::no_suffix;
    py::enum_< timestr::rel_t >( m, "rel_t" );
    m.attr( "rel" ) = timestr::rel;
    py::enum_< timestr::abs_t >( m, "abs_t" );
    m.attr( "abs" ) = timestr::abs;

    m.def( "timestr", []( dds_nsec t ) { return timestr( t ).to_string(); } );
    m.def( "timestr", []( dds_nsec t, timestr::no_suffix_t ) { return timestr( t, timestr::no_suffix ).to_string(); } );
    m.def( "timestr", []( dds_nsec dt, timestr::rel_t ) { return timestr( dt, timestr::rel ).to_string(); } );
    m.def( "timestr", []( dds_nsec dt, timestr::rel_t, timestr::no_suffix_t ) { return timestr( dt, timestr::rel, timestr::no_suffix ).to_string(); } );
    m.def( "timestr", []( dds_nsec t1, dds_nsec t2 ) { return timestr( t1, t2 ).to_string(); } );
    m.def( "timestr", []( dds_nsec t1, dds_nsec t2, timestr::no_suffix_t ) { return timestr( t1, t2, timestr::no_suffix ).to_string(); } );

    m.def( "timestr", []( dds_time t, dds_time start, timestr::no_suffix_t ) { return timestr( t, start, timestr::no_suffix ).to_string(); } );
    m.def( "timestr", []( dds_time t, dds_nsec start, timestr::no_suffix_t ) { return timestr( t, start, timestr::no_suffix ).to_string(); } );
    m.def( "timestr", []( dds_nsec t, dds_time start, timestr::no_suffix_t ) { return timestr( t, start, timestr::no_suffix ).to_string(); } );
    m.def( "timestr", []( dds_time t, timestr::no_suffix_t ) { return timestr( t, timestr::no_suffix ).to_string(); } );
    
    m.def( "timestr", []( dds_time t, dds_time start ) { return timestr( t, start ).to_string(); } );
    m.def( "timestr", []( dds_time t, dds_nsec start ) { return timestr( t, start ).to_string(); } );
    m.def( "timestr", []( dds_nsec t, dds_time start ) { return timestr( t, start ).to_string(); } );
    m.def( "timestr", []( dds_time t ) { return timestr( t ).to_string(); } );


    typedef std::shared_ptr< dds_topic > flexible_msg_create_topic( std::shared_ptr< dds_participant > const &,
                                                                    char const * );
    py::class_< flexible_msg >( m, "flexible_msg" )
        .def( py::init<>() )
        .def( py::init( []( std::string const & json_string ) {
            return flexible_msg( flexible_msg::data_format::JSON, nlohmann::json::parse( json_string ) );
        } ) )
        .def_readwrite( "data_format", &flexible_msg::_data_format )
        .def_readwrite( "version", &flexible_msg::_version )
        .def_readwrite( "data", &flexible_msg::_data )
        .def( "invalidate", &flexible_msg::invalidate )
        .def( "is_valid", &flexible_msg::is_valid )
        .def( "__nonzero__", &flexible_msg::is_valid )  // Called to implement truth value testing in Python 2
        .def( "__bool__", &flexible_msg::is_valid )     // Called to implement truth value testing in Python 3
        .def( "__repr__",
              []( flexible_msg const & self ) {
                  std::ostringstream os;
                  os << "<" SNAME ".flexible_msg ";
                  switch( self._data_format )
                  {
                  case flexible_msg::data_format::JSON:
                      os << "JSON";
                      break;
                  case flexible_msg::data_format::CBOR:
                      os << "CBOR";
                      break;
                  case flexible_msg::data_format::CUSTOM:
                      os << "CUSTOM";
                      break;
                  default:
                      os << "UNKNOWN(" << (int)self._data_format << ")";
                      break;
                  }
                  os << ' ';
                  if( ! self.is_valid() )
                      os << "INVALID";
                  else
                  {
                      os << self._data.size();
                      switch( self._data_format )
                      {
                      case flexible_msg::data_format::JSON:
                          os << ' ';
                          os << std::string( (char const *)self._data.data(), self._data.size() );
                          break;
                      default:
                          break;
                      }
                  }
                  os << ">";
                  return os.str();
              } )
        .def_static(
            "take_next",
            []( dds_topic_reader & reader, SampleInfo * sample ) {
                auto actual_type = reader.topic()->get()->get_type_name();
                if( actual_type != flexible_msg::type().getName() )
                    throw std::runtime_error( "can't initialize raw::flexible from " + actual_type );
                flexible_msg data;
                if( ! flexible_msg::take_next( reader, &data, sample ) )
                    assert( ! data.is_valid() );
                return data;
            },
            py::arg( "reader" ), py::arg( "sample" ) = nullptr,
            py::call_guard< py::gil_scoped_release >() )
        .def_static( "create_topic", static_cast<flexible_msg_create_topic *>( &flexible_msg::create_topic ))
        .def( "json_data", &flexible_msg::json_data )
        .def( "json_string",
              []( flexible_msg const & self )
              { return std::string( (char const *)self._data.data(), self._data.size() ); } )
        .def( "write_to", &flexible_msg::write_to, py::call_guard< py::gil_scoped_release >() );


    using image_msg = realdds::topics::device::image_msg;
    py::class_< image_msg, std::shared_ptr< image_msg > >( m, "image_msg" )
        .def( py::init<>() )
        .def_readwrite( "frame_id", &image_msg::frame_id )
        .def_readwrite( "data", &image_msg::raw_data )
        .def_readwrite( "width", &image_msg::width )
        .def_readwrite( "height", &image_msg::height )
        .def_readwrite( "timestamp", &image_msg::timestamp )
        .def( "__repr__",
              []( image_msg const & self )
              {
                  std::ostringstream os;
                  os << "<" SNAME ".image_msg";
                  if( self.width > 0 && self.height > 0 )
                  {
                      os << ' ' << self.width << 'x' << self.height;
                      os << 'x' << (self.raw_data.size() / (self.width * self.height));
                  }
                  os << " @ " << realdds::time_to_string( self.timestamp );
                  os << ">";
                  return os.str();
              } )
        .def_static(
            "take_next",
            []( dds_topic_reader & reader, SampleInfo * sample )
            {
                auto actual_type = reader.topic()->get()->get_type_name();
                if( actual_type != image_msg::type().getName() )
                    throw std::runtime_error( "can't initialize raw::image from " + actual_type );
                image_msg data;
                if( ! image_msg::take_next( reader, &data, sample ) )
                    assert( ! data.is_valid() );
                return data;
            },
            py::arg( "reader" ),
            py::arg( "sample" ) = nullptr,
            py::call_guard< py::gil_scoped_release >() )
        .def_static( "create_topic", &image_msg::create_topic )
        /*.def("write_to", &image_msg::write_to, py::call_guard< py::gil_scoped_release >())*/;


    using realdds::dds_device_broadcaster;
    py::class_< dds_device_broadcaster >( m, "device_broadcaster" )
        .def( py::init< std::shared_ptr< dds_publisher > const &, device_info const & >() );

    using realdds::dds_option_range;
    py::class_< dds_option_range >( m, "dds_option_range" )
        .def( py::init<>() )
        .def_readwrite( "min", &dds_option_range::min )
        .def_readwrite( "max", &dds_option_range::max )
        .def_readwrite( "step", &dds_option_range::step )
        .def_readwrite( "default_value", &dds_option_range::default_value );

    using realdds::dds_option;
    py::class_< dds_option, std::shared_ptr< dds_option > >( m, "dds_option" )
        .def( py::init< std::string const &, std::string const & >() )
        .def( "get_name", &dds_option::get_name )
        .def( "owner_name", &dds_option::owner_name )
        .def( "get_value", &dds_option::get_value )
        .def( "set_value", &dds_option::set_value )
        .def( "get_range", &dds_option::get_range )
        .def( "set_range", &dds_option::set_range )
        .def( "get_description", &dds_option::get_description )
        .def( "set_description", &dds_option::set_description )
        .def( "to_json", []( dds_option const & self ) { return self.to_json().dump(); } );

    using realdds::dds_stream_format;
    py::class_< dds_stream_format >( m, "stream_format" )
        .def( py::init<>() )
        .def( py::init< std::string const & >() )
        .def( "is_valid", &dds_stream_format::is_valid )
        .def( "to_rs2", &dds_stream_format::to_rs2 )
        .def( "from_rs2", &dds_stream_format::from_rs2 )
        .def( "__nonzero__", &dds_stream_format::is_valid )  // Called to implement truth value testing in Python 2
        .def( "__bool__", &dds_stream_format::is_valid )     // Called to implement truth value testing in Python 3
        .def( "__repr__", []( dds_stream_format const & self ) { return self.to_string(); } );

    using realdds::dds_stream_profile;
    py::class_< dds_stream_profile, std::shared_ptr< dds_stream_profile > > stream_profile_base( m, "stream_profile" );
    stream_profile_base
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
        .def( py::init< int16_t, dds_stream_format, uint16_t, uint16_t >() )
        .def( "width", &dds_video_stream_profile::width )
        .def( "height", &dds_video_stream_profile::height );

    using realdds::dds_motion_stream_profile;
    py::class_< dds_motion_stream_profile, std::shared_ptr< dds_motion_stream_profile > >( m, "motion_stream_profile", stream_profile_base )
        .def( py::init< int16_t, dds_stream_format >() );

    using realdds::dds_stream_base;
    py::class_< dds_stream_base, std::shared_ptr< dds_stream_base > > stream_base( m, "stream_base" );
    stream_base
        .def( "name", &dds_stream_base::name )
        .def( "sensor_name", &dds_stream_base::sensor_name )
        .def( "type_string", &dds_stream_base::type_string )
        .def( "profiles", &dds_stream_base::profiles )
        .def( "enable_metadata", &dds_stream_base::enable_metadata )
        .def( "init_profiles", &dds_stream_base::init_profiles )
        .def( "init_options", &dds_stream_base::init_options )
        .def( "default_profile_index", &dds_stream_base::default_profile_index )
        .def( "default_profile", &dds_stream_base::default_profile )
        .def( "options", &dds_stream_base::options )
        .def( "is_open", &dds_stream_base::is_open )
        .def( "is_streaming", &dds_stream_base::is_streaming )
        .def( "get_topic", &dds_stream_base::get_topic );

    using realdds::dds_stream_server;
    py::class_< dds_stream_server, std::shared_ptr< dds_stream_server > > stream_server_base( m, "stream_server", stream_base );
    stream_server_base  //
        .def( "on_readers_changed", &dds_stream_server::on_readers_changed )
        .def( "open", &dds_stream_server::open )
        .def( "start_streaming",
              []( dds_stream_server & self, dds_stream_format format, int width, int height ) {
                  self.start_streaming( { format, height, width } );
              } )
        .def( "stop_streaming", &dds_stream_server::stop_streaming )
        .def( "__repr__",
              []( dds_stream_server const & self )
              {
                  std::ostringstream os;
                  os << "<" SNAME "." << self.type_string() << "_stream_server \"" << self.name() << "\"";
                  os << " " << self.profiles().size() << "[" << self.default_profile_index() << "] profiles";
                  os << '>';
                  return os.str();
              } );

    using realdds::dds_video_stream_server;
    py::class_< dds_video_stream_server, std::shared_ptr< dds_video_stream_server > >
        video_stream_server_base( m, "video_stream_server", stream_server_base );
    video_stream_server_base
        .def( "set_intrinsics", &dds_video_stream_server::set_intrinsics )
        .def( "publish_image",
              []( dds_video_stream_server & self, image_msg const & img, uint64_t frame_id )
              { self.publish( img.raw_data.data(), img.raw_data.size(), frame_id ); } );

    using realdds::dds_depth_stream_server;
    py::class_< dds_depth_stream_server, std::shared_ptr< dds_depth_stream_server > >( m, "depth_stream_server", video_stream_server_base )
        .def( py::init< std::string const &, std::string const & >(), "stream_name"_a, "sensor_name"_a );

    using realdds::dds_ir_stream_server;
    py::class_< dds_ir_stream_server, std::shared_ptr< dds_ir_stream_server > >( m, "ir_stream_server", video_stream_server_base )
        .def( py::init< std::string const&, std::string const& >(), "stream_name"_a, "sensor_name"_a );

    using realdds::dds_color_stream_server;
    py::class_< dds_color_stream_server, std::shared_ptr< dds_color_stream_server > >( m, "color_stream_server", video_stream_server_base )
        .def( py::init< std::string const&, std::string const& >(), "stream_name"_a, "sensor_name"_a );

    using realdds::dds_fisheye_stream_server;
    py::class_< dds_fisheye_stream_server, std::shared_ptr< dds_fisheye_stream_server > >( m, "fisheye_stream_server", video_stream_server_base )
        .def( py::init< std::string const&, std::string const& >(), "stream_name"_a, "sensor_name"_a );

    using realdds::dds_confidence_stream_server;
    py::class_< dds_confidence_stream_server, std::shared_ptr< dds_confidence_stream_server > >( m, "confidence_stream_server", video_stream_server_base )
        .def( py::init< std::string const&, std::string const& >(), "stream_name"_a, "sensor_name"_a );

    using realdds::dds_motion_stream_server;
    py::class_< dds_motion_stream_server, std::shared_ptr< dds_motion_stream_server > >
        motion_stream_server_base( m, "motion_stream_server", stream_server_base );
    motion_stream_server_base
        .def( "set_intrinsics", &dds_motion_stream_server::set_intrinsics );

    using realdds::dds_accel_stream_server;
    py::class_< dds_accel_stream_server, std::shared_ptr< dds_accel_stream_server > >( m, "accel_stream_server", motion_stream_server_base )
        .def( py::init< std::string const &, std::string const & >(), "stream_name"_a, "sensor_name"_a );

    using realdds::dds_gyro_stream_server;
    py::class_< dds_gyro_stream_server, std::shared_ptr< dds_gyro_stream_server > >( m, "gyro_stream_server", motion_stream_server_base )
        .def( py::init< std::string const&, std::string const& >(), "stream_name"_a, "sensor_name"_a );

    using realdds::dds_pose_stream_server;
    py::class_< dds_pose_stream_server, std::shared_ptr< dds_pose_stream_server > >( m, "pose_stream_server", motion_stream_server_base )
        .def( py::init< std::string const&, std::string const& >(), "stream_name"_a, "sensor_name"_a );

    using realdds::dds_device_server;
    py::class_< dds_device_server, std::shared_ptr< dds_device_server > >( m, "device_server" )
        .def( py::init< std::shared_ptr< dds_participant > const&, std::string const& >() )
        .def( "init", &dds_device_server::init )
        .def( "streams",
              []( dds_device_server const & self )
              {
                  std::vector< std::shared_ptr< dds_stream_server > > streams;
                  for( auto const & name2stream : self.streams() )
                      streams.push_back( name2stream.second );
                  return streams;
              } )
        .def( "publish_metadata", &dds_device_server::publish_metadata, py::call_guard< py::gil_scoped_release >() )
        .def( "broadcast", &dds_device_server::broadcast );

    using realdds::dds_stream;
    py::class_< dds_stream, std::shared_ptr< dds_stream > > stream_client_base( m, "stream", stream_base );
    stream_client_base  //
        .def( "open", &dds_stream::open )
        .def( "close", &dds_stream::close )
        .def( "is_open", &dds_stream::is_open )
        .def( "start_streaming", &dds_stream::start_streaming )
        .def( "stop_streaming", &dds_stream::stop_streaming )
        .def( "__repr__", []( dds_stream const & self ) {
            std::ostringstream os;
            os << "<" SNAME "." << self.type_string() << "_stream \"" << self.name() << "\"";
            os << " " << self.profiles().size() << "[" << self.default_profile_index() << "] profiles";
            os << '>';
            return os.str();
        } );

    using realdds::dds_video_stream;
    py::class_< dds_video_stream, std::shared_ptr< dds_video_stream > >
        video_stream_client_base( m, "video_stream", stream_client_base );
    video_stream_client_base  //
        .def( "set_intrinsics", &dds_video_stream::set_intrinsics )
        .def( FN_FWD( dds_video_stream,
                      on_data_available,
                      ( dds_video_stream &, image_msg && ),
                      ( image_msg && i ),
                      callback( self, std::move( i ) ); ) );

    using realdds::dds_depth_stream;
    py::class_< dds_depth_stream, std::shared_ptr< dds_depth_stream > >( m, "depth_stream", video_stream_client_base )
        .def( py::init< std::string const &, std::string const & >(), "stream_name"_a, "sensor_name"_a );

    using realdds::dds_ir_stream;
    py::class_< dds_ir_stream, std::shared_ptr< dds_ir_stream > >( m, "ir_stream", video_stream_client_base )
        .def( py::init< std::string const &, std::string const & >(), "stream_name"_a, "sensor_name"_a );

    using realdds::dds_color_stream;
    py::class_< dds_color_stream, std::shared_ptr< dds_color_stream > >( m, "color_stream", video_stream_client_base )
        .def( py::init< std::string const &, std::string const & >(), "stream_name"_a, "sensor_name"_a );

    using realdds::dds_fisheye_stream;
    py::class_< dds_fisheye_stream, std::shared_ptr< dds_fisheye_stream > >( m, "fisheye_stream", video_stream_client_base )
        .def( py::init< std::string const &, std::string const & >(), "stream_name"_a, "sensor_name"_a );

    using realdds::dds_confidence_stream;
    py::class_< dds_confidence_stream, std::shared_ptr< dds_confidence_stream > >( m, "confidence_stream", video_stream_client_base )
        .def( py::init< std::string const &, std::string const & >(), "stream_name"_a, "sensor_name"_a );

    using realdds::dds_motion_stream;
    py::class_< dds_motion_stream, std::shared_ptr< dds_motion_stream > >
        motion_stream_client_base( m, "motion_stream", stream_client_base );
    motion_stream_client_base
        .def( "set_intrinsics", &dds_motion_stream::set_intrinsics );

    using realdds::dds_accel_stream;
    py::class_< dds_accel_stream, std::shared_ptr< dds_accel_stream > >( m, "accel_stream", motion_stream_client_base )
        .def( py::init< std::string const &, std::string const & >(), "stream_name"_a, "sensor_name"_a );

    using realdds::dds_gyro_stream;
    py::class_< dds_gyro_stream, std::shared_ptr< dds_gyro_stream > >( m, "gyro_stream", motion_stream_client_base )
        .def( py::init< std::string const &, std::string const & >(), "stream_name"_a, "sensor_name"_a );

    using realdds::dds_pose_stream;
    py::class_< dds_pose_stream, std::shared_ptr< dds_pose_stream > >( m, "pose_stream", motion_stream_client_base )
        .def( py::init< std::string const &, std::string const & >(), "stream_name"_a, "sensor_name"_a );

    using realdds::dds_device;
    py::class_< dds_device,
        std::shared_ptr< dds_device >  // handled with a shared_ptr
    >( m, "device" )
        .def( py::init( &dds_device::create ) )
        .def( "device_info", &dds_device::device_info )
        .def( "participant", &dds_device::participant )
        .def( "guid", &dds_device::guid )
        .def( "is_running", &dds_device::is_running )
        .def( "run", &dds_device::run, py::call_guard< py::gil_scoped_release >() )
        .def( FN_FWD( dds_device,
                      on_metadata_available,
                      ( dds_device &, py::object && ),
                      ( nlohmann::json && j ),
                      callback( self, json_to_py( j ) ); ) )
        .def( "n_streams", &dds_device::number_of_streams )
        .def( "streams",
              []( dds_device const & self ) {
                  std::vector< std::shared_ptr< dds_stream > > streams;
                  self.foreach_stream(
                      [&]( std::shared_ptr< dds_stream > const & stream ) { streams.push_back( stream ); } );
                  return streams;
              } )
        .def( "options",
              []( dds_device const & self ) {
                  std::vector< std::shared_ptr< dds_option > > options;
                  self.foreach_option( [&]( std::shared_ptr< dds_option > const & option ) { options.push_back( option ); } );
                  return options;
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

    using realdds::dds_stream_sensor_bridge;
    py::class_< dds_stream_sensor_bridge >( m, "stream_sensor_bridge" )
        .def( py::init<>() )
        .def( FN_FWD( dds_stream_sensor_bridge,
                      on_start_sensor,
                      (std::string const &, std::vector< std::shared_ptr< realdds::dds_stream_profile > > const &),
                      ( std::string const & sensor_name,
                        std::vector< std::shared_ptr< realdds::dds_stream_profile > > const & profiles ),
                      callback( sensor_name, profiles ); ) )
        .def( FN_FWD( dds_stream_sensor_bridge,
                      on_stop_sensor,
                      (std::string const &),
                      ( std::string const & sensor_name ),
                      callback( sensor_name ); ) )
        .def( FN_FWD( dds_stream_sensor_bridge,
                      on_readers_changed,
                      (std::shared_ptr< dds_stream_server > const &, int),
                      ( std::shared_ptr< dds_stream_server > const & server, int n_readers ),
                      callback( server, n_readers ); ) )
        .def( FN_FWD( dds_stream_sensor_bridge,
                      on_error,
                      (std::string const &),
                      ( std::string const & error_string ),
                      callback( error_string ); ) )
        .def( "init", &dds_stream_sensor_bridge::init )
        .def( "reset", &dds_stream_sensor_bridge::reset, py::arg( "by_force" ) = false )
        .def( "open", &dds_stream_sensor_bridge::open )
        .def( "close", &dds_stream_sensor_bridge::close )
        .def( "add_implicit_profiles", &dds_stream_sensor_bridge::add_implicit_profiles )
        .def( "commit", &dds_stream_sensor_bridge::commit )
        .def( "is_streaming", &dds_stream_sensor_bridge::is_streaming );

    // Custom syncer with its own frame management over image_msg
    struct dds_metadata_syncer : realdds::dds_metadata_syncer
    {
        typedef std::shared_ptr< image_msg > frame_type;  // the base case for image_msg (see above)

    private:
        typedef realdds::dds_metadata_syncer super;

        static void frame_releaser( void * frame )
        {
            // frame is really a pointer to a shared object that we create on the heap
            delete static_cast< frame_type * >( frame );
        }

    public:
        dds_metadata_syncer()
        {
            on_frame_release( frame_releaser );
        }

        void enqueue_frame( key_type key, frame_type const & img )
        {
            // We need to keep the image alive and somehow point to it at the same time
            super::enqueue_frame( key, hold( new std::shared_ptr< image_msg >( img ) ) );
        }

        frame_type get_frame( frame_holder const & fh ) const
        {
            return *static_cast< frame_type * >( fh.get() );
        }
    };

    py::class_< dds_metadata_syncer > metadata_syncer( m, "metadata_syncer" );
    metadata_syncer  //
        .def( py::init<>() )
        .def( FN_FWD( dds_metadata_syncer,
                      on_frame_ready,
                      ( dds_metadata_syncer::frame_type, nlohmann::json && ),
                      ( dds_metadata_syncer::frame_holder && fh, nlohmann::json && metadata ),
                      callback( self.get_frame( fh ), std::move( metadata ) ); ) )
        .def( FN_FWD( dds_metadata_syncer,
                      on_metadata_dropped,
                      ( dds_metadata_syncer::key_type, nlohmann::json && ),
                      ( dds_metadata_syncer::key_type key, nlohmann::json && metadata ),
                      callback( key, std::move( metadata ) ); ) )
        .def( "enqueue_frame", &dds_metadata_syncer::enqueue_frame )
        .def( "enqueue_metadata", &dds_metadata_syncer::enqueue_metadata )
        .def( "clear", &dds_metadata_syncer::clear );
    metadata_syncer.attr( "max_frame_queue_size" ) = dds_metadata_syncer::max_frame_queue_size;
    metadata_syncer.attr( "max_md_queue_size" ) = dds_metadata_syncer::max_md_queue_size;
}
