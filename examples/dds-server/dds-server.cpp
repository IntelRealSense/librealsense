// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include "dds-server.h"

#include <array>

#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/qos/PublisherQos.hpp>

// We align the DDS topic name to ROS2 as it expect the 'rt/' prefix for the topic name
#define ROS2_PREFIX( name ) std::string( "rt/" ).append( name )

using namespace eprosima::fastdds::dds;

dds_server::dds_server()
    : _participant( nullptr )
    , _publisher( nullptr )
    , _topic( nullptr )
    , _type_support_ptr( new devicesPubSubType() )
{
}

void dds_server::init()
{
    DomainParticipantQos pqos;
    pqos.name( "dds-server" );
    _participant = DomainParticipantFactory::get_instance()->create_participant( 0, pqos );

    if( _participant == nullptr )
    {
        throw std::runtime_error( "Error creating a DDS participant" );
    }

    // Registering the topic type enables topic instance creation by factory
    _type_support_ptr.register_type( _participant );  // TODO - should we use ROS2_PREFIX here? or
                                                      // generate from IDL with -typeros2 flag?

    _publisher = _participant->create_publisher( PUBLISHER_QOS_DEFAULT, nullptr );

    if( _publisher == nullptr )
    {
        throw std::runtime_error( "Error creating a DDS publisher" );
    }

    _topic = _participant->create_topic( ROS2_PREFIX( "Devices" ),
                                         _type_support_ptr->getName(),
                                         TOPIC_QOS_DEFAULT );

    if( _topic == nullptr )
    {
        throw std::runtime_error( "Error creating a DDS Devices topic" );
    }

    _ctx.set_devices_changed_callback( [&]( rs2::event_information & info ) {
        // Remove disconnected devices
        std::vector<std::string> devices_to_remove;
        for( auto dev_info : _devices_writers )
        {
            auto & dev = dev_info.second.device;
            auto device_name = dev.get_info( RS2_CAMERA_INFO_NAME );
            
            if( info.was_removed( dev ) )
            {
                devices_to_remove.push_back(device_name);
            }
        }

        for( auto dev_to_remove : devices_to_remove )
        {
             _publisher->delete_datawriter( _devices_writers[dev_to_remove].data_writer );
             std::cout << "removing device: " << dev_to_remove << std::endl;
            _devices_writers.erase( dev_to_remove );
        }

        //// Add new connected devices
        for( auto && dev : info.get_new_devices() )
        {
            auto device_name = dev.get_info( RS2_CAMERA_INFO_NAME );
            // Create a data writer for the topic
            DataWriterQos wqos = DATAWRITER_QOS_DEFAULT;
            wqos.reliability().kind = RELIABLE_RELIABILITY_QOS;
            wqos.durability().kind = VOLATILE_DURABILITY_QOS;
            wqos.data_sharing().automatic();
            wqos.ownership().kind = EXCLUSIVE_OWNERSHIP_QOS;
            std::shared_ptr< dds_serverListener > writer_listener = std::make_shared< dds_serverListener >();
            _devices_writers[device_name]
                = { dev,
                    _publisher->create_datawriter( _topic, wqos, writer_listener.get() ),
                    writer_listener };

            if( _devices_writers[device_name].data_writer == nullptr )
            {
                std::cout <<   "Error creating a DDS writer" << std::endl;
            }

            strcpy( _devicesTopic.name().data(), device_name );
            std::cout << "adding device: " << device_name << std::endl;
            // TODO, change with `readers match` logic
            std::this_thread::sleep_for(std::chrono::seconds(1));
            if( ! _devices_writers[device_name].data_writer->write( &_devicesTopic ) )
            {
                std::cout << "Error writing add new device topic for: " << device_name << std::endl;
            }
        }
    } );
}

dds_server::~dds_server()
{
    for( auto device_writer : _devices_writers )
    {
        auto & dev_writer = device_writer.second.data_writer;
        if( dev_writer )
            _publisher->delete_datawriter( dev_writer );
    }
    _devices_writers.clear();

    if( _topic != nullptr )
    {
        _participant->delete_topic( _topic );
    }
    if( _publisher != nullptr )
    {
        _participant->delete_publisher( _publisher );
    }
    DomainParticipantFactory::get_instance()->delete_participant( _participant );
}


void dds_server::dds_serverListener::on_publication_matched(
    eprosima::fastdds::dds::DataWriter *,
    const eprosima::fastdds::dds::PublicationMatchedStatus & info )
{
    if( info.current_count_change == 1 )
    {
        _matched = info.total_count;
    }
    else if( info.current_count_change == -1 )
    {
        _matched = info.total_count;
    }
    else
    {
        std::cout << std::to_string( info.current_count_change )
                        << " is not a valid value for on_publication_matched" << std::endl;
    }
}


int main( int argc, char * argv[] )
try
{
    std::cout << "RealSense DDS Server example is on, press any key to stop.." << std::endl
              << std::endl;
    dds_server my_dds_server;
    my_dds_server.init();
    std::cin.ignore();
    return EXIT_SUCCESS;
}
catch( const rs2::error & e )
{
    std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args()
              << "):\n    " << e.what() << std::endl;
    return EXIT_FAILURE;
}
catch( const std::exception & e )
{
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}
