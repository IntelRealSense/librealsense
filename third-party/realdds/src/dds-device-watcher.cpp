// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <realdds/dds-device-watcher.h>
#include <realdds/dds-topic.h>
#include <realdds/dds-topic-reader-thread.h>
#include <realdds/dds-device.h>
#include <realdds/dds-utilities.h>
#include <realdds/dds-guid.h>
#include <realdds/topics/dds-topic-names.h>
#include <realdds/topics/flexible-msg.h>
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

                // NOTE: the GUID we get here is the writer on the device-info, used nowhere again in the system, which
                // can therefore be confusing. It is not the "server GUID", but can still be used to uniquely identify
                // the device instance since there should be one writer per device.
                dds_guid guid;
                eprosima::fastrtps::rtps::iHandle2GUID( guid, info.publication_handle );

                std::shared_ptr< dds_device > device;
                {
                    std::lock_guard< std::mutex > lock( _devices_mutex );
                    auto it = _dds_devices.find( guid );
                    if( it != _dds_devices.end() )
                        device = it->second;
                }

                auto j = msg.json_data();
                if( j.find( "stopping" ) != j.end() )
                {
                    // This device is stopping for whatever reason (e.g., HW reset); remove it
                    LOG_DEBUG( "DDS device (from " << _participant->print( guid ) << ") is stopping" );
                    // TODO notify the device?
                    remove_device( guid );
                    continue;
                }

                if( device )
                    // We already know about this device; likely this was a broadcast meant for someone else
                    continue;

                topics::device_info device_info = topics::device_info::from_json( j );

                LOG_DEBUG( "DDS device (from " << _participant->print( guid ) << ") detected: " << j.dump( 4 ) );

                // Add a new device record into our dds devices map
                device = std::make_shared< dds_device >( _participant, device_info );
                {
                    std::lock_guard< std::mutex > lock( _devices_mutex );
                    _dds_devices[guid] = device;
                }

                // NOTE: device removals are handled via the writer-removed notification; see the
                // listener callback in init().
                if( _on_device_added )
                {
                    std::thread(
                        [device, on_device_added = _on_device_added]() {  //
                            on_device_added( device );
                        } )
                        .detach();
                }
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
                                                 << realdds::print_guid( _participant->guid() ) );
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
        _participant->create_listener( &_listener )
            ->on_writer_removed( [this]( dds_guid guid, char const * ) { remove_device( guid ); } );

    if( ! _device_info_topic->is_running() )
        _device_info_topic->run( dds_topic_reader::qos() );
}


void dds_device_watcher::remove_device( dds_guid const & guid )
{
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
    std::thread(
        [device, on_device_removed = _on_device_removed]()
        {
            if( on_device_removed )
                on_device_removed( device );
            // If we're holding the device, it will get destroyed here, from another thread.
            // Not sure why, but if we delete the outside this thread (in the listener callback), it
            // will cause some sort of invalid state in DDS. The thread will get killed and we won't get
            // any notification of the remote participant getting removed... and the process will even
            // hang on exit.
        } )
        .detach();
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

