// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <realdds/dds-device-watcher.h>
#include <realdds/dds-topic.h>
#include <realdds/dds-topic-reader-thread.h>
#include <realdds/dds-device.h>
#include <realdds/dds-utilities.h>
#include <realdds/dds-guid.h>
#include <realdds/topics/dds-topic-names.h>
#include <realdds/topics/flexible/flexible-msg.h>
#include <realdds/topics/device-info-msg.h>

#include <rsutils/json.h>

#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>


using namespace eprosima::fastdds::dds;
using namespace realdds;


dds_device_watcher::dds_device_watcher( std::shared_ptr< dds_participant > const & participant )
    : _device_info_topic(
        new dds_topic_reader_thread( topics::flexible_msg::create_topic( participant, topics::DEVICE_INFO_TOPIC_NAME ) ) )
    , _participant( participant )
{
    _device_info_topic->on_data_available(
        [this]()
        {
            topics::flexible_msg msg;
            eprosima::fastdds::dds::SampleInfo info;
            while( topics::flexible_msg::take_next( *_device_info_topic, &msg, &info ) )
            {
                if( ! msg.is_valid() )
                    continue;

                topics::device_info device_info = topics::device_info::from_json( msg.json_data() );

                eprosima::fastrtps::rtps::GUID_t guid;
                eprosima::fastrtps::rtps::iHandle2GUID( guid, info.publication_handle );

                LOG_DEBUG( "DDS device (" << _participant->print( guid ) << ") detected:"
                                          << "\n\tName: " << device_info.name
                                          << "\n\tSerial: " << device_info.serial
                                          << "\n\tProduct line: " << device_info.product_line
                                          << "\n\tProduct ID: " << device_info.product_id
                                          << "\n\tTopic root: " << device_info.topic_root
                                          << "\n\tLocked: " << ( device_info.locked ? "yes" : "no" ) );

                // Add a new device record into our dds devices map
                auto device = dds_device::create( _participant, guid, device_info );
                {
                    std::lock_guard< std::mutex > lock( _devices_mutex );
                    _dds_devices[guid] = device;
                }

                // NOTE: device removals are handled via the writer-removed notification; see the
                // listener callback in init().
                if( _on_device_added )
                    _on_device_added( device );
            }
        } );

    if( ! _participant->is_valid() )
        DDS_THROW( runtime_error, "participant was not initialized" );
}


bool dds_device_watcher::is_stopped() const
{
    return ! _device_info_topic->is_running();
}


void dds_device_watcher::start()
{
    stop();
    if( ! _device_info_topic->is_running() )
    {
        // Get all sensors & profiles data
        // TODO: not sure we want to do it here in the C'tor, 
        // it takes time and keeps the 'dds_device_server' busy
        init();
    }
    LOG_DEBUG( "DDS device watcher started on '" << _participant->get()->get_qos().name() << "' "
                                                 << realdds::print( _participant->guid() ) );
}

void dds_device_watcher::stop()
{
    if( ! is_stopped() )
    {
        _device_info_topic->stop();
        //_callback_inflight.wait_until_empty();
        LOG_DEBUG( "DDS device watcher stopped" );
    }
}


dds_device_watcher::~dds_device_watcher()
{
    stop();
}

void dds_device_watcher::init()
{
    if( ! _listener )
        _participant->create_listener( &_listener )->on_writer_removed( [this]( dds_guid guid, char const * ) {
            std::shared_ptr< dds_device > device;
            {
                std::lock_guard< std::mutex > lock( _devices_mutex );
                auto it = _dds_devices.find( guid );
                if( it == _dds_devices.end() )
                    return;
                device = it->second;
                _dds_devices.erase( it );
            }
            // rest must happen outside the mutex
            std::thread( [device, on_device_removed = _on_device_removed]() {
                if( on_device_removed )
                    on_device_removed( device );
                // If we're holding the device, it will get destroyed here, from another thread.
                // Not sure why, but if we delete the outside this thread (in the listener callback), it will cause some
                // sort of invalid state in DDS. The thread will get killed and we won't get any notification of the
                // remote participant getting removed... and the process will even hang on exit.
            } ).detach();
        } );

    if( ! _device_info_topic->is_running() )
        _device_info_topic->run( dds_topic_reader::qos() );

    LOG_DEBUG( "DDS device watcher is running" );
}


bool dds_device_watcher::foreach_device(
    std::function< bool( std::shared_ptr< dds_device > const & ) > fn ) const
{
    std::lock_guard< std::mutex > lock( _devices_mutex );
    for( auto && guid_to_dev_info : _dds_devices )
    {
        if( ! fn( guid_to_dev_info.second ) )
            return false;
    }
    return true;
}

