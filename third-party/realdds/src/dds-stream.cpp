// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <realdds/dds-stream.h>
#include "dds-stream-impl.h"

#include <realdds/dds-topic.h>
#include <realdds/dds-topic-reader.h>
#include <realdds/dds-subscriber.h>
#include <realdds/topics/image/image-msg.h>
#include <realdds/topics/flexible/flexible-msg.h>
#include <realdds/dds-exceptions.h>

#include <rsutils/json.h>

#include <fastdds/dds/subscriber/SampleInfo.hpp>

#include <deque>


namespace realdds {

// Image data (pixels) and metadata are sent as two seperate streams.
// They are synchronized using frame_id for matching data+metadata sets.
class frame_metadata_syncer
{
public:
    // We don't want the queue to get large, it means lots of drops and data that we store to (probably) throw later
    static constexpr size_t max_queue_size = 8;

    void enqueue_image( topics::device::image && image )
    {
        std::lock_guard< std::mutex > lock( _queues_lock );
        if( _image_queue.size() < max_queue_size )
        {
            _image_queue.push_back( std::move( image ) );
            search_for_match(); //Call under lock
        }
        else
            _image_queue.clear();
    }

    void enqueue_metadata( topics::flexible_msg && md )
    {
        std::lock_guard< std::mutex > lock( _queues_lock );
        if( _metadata_queue.size() < max_queue_size )
        {
            _metadata_queue.push_back( std::move( md ) );
            search_for_match(); //Call under lock
        }
        else
            _metadata_queue.clear();
    }

    dds_stream::on_data_available_callback get_callback() { return _cb; }

    void reset( dds_stream::on_data_available_callback cb = nullptr)
    {
        std::lock_guard< std::mutex > lock( _queues_lock );
        _image_queue.clear();
        _metadata_queue.clear();
        _cb = cb;
    }

private:
    //Call under lock
    void search_for_match()
    {
        // Wait for image + metadata set
        if( _image_queue.empty() || _metadata_queue.empty() )
            return;

        // Sync using frame ID. Frame IDs are assumed to be increasing with time
        // If one of the set is lost throw the other.
        std::string image_frame_id = _image_queue.front().frame_id;
        std::string md_frame_id = rsutils::json::get< std::string >( _metadata_queue.front().json_data(), "frame_id" );

        while( image_frame_id > md_frame_id && _metadata_queue.size() > 1 )
        {
            // Metadata without frame, remove it from queue and check next
            _metadata_queue.pop_front();
            md_frame_id = rsutils::json::get< std::string >( _metadata_queue.front().json_data(), "frame_id" );
        }

        while( md_frame_id > image_frame_id && _image_queue.size() > 1 )
        {
            // Image without metadata, remove it from queue and check next
            _image_queue.pop_front();
            image_frame_id = _image_queue.front().frame_id;
        }

        if( image_frame_id == md_frame_id )
            handle_match();
    }

    //Call under lock, will pass callback to dispatcher to avoid delaying enqueing threads
    void handle_match()
    {
        topics::device::image & image = _image_queue.front();
        topics::flexible_msg & md = _metadata_queue.front();

        if( _cb )
            _cb( std::move( image ), std::move( md ) );

        _image_queue.pop_front();
        _metadata_queue.pop_front();
    }

    dds_stream::on_data_available_callback _cb = nullptr;

    std::deque< topics::device::image > _image_queue;
    std::deque< topics::flexible_msg > _metadata_queue;
    std::mutex _queues_lock;
};


dds_stream::dds_stream( std::string const & stream_name, std::string const & sensor_name )
    : super( stream_name, sensor_name )
    , _syncer( std::make_shared< frame_metadata_syncer >() )
{
}

bool dds_stream::is_streaming() const
{
    return _syncer->get_callback() != nullptr;
}

void dds_stream::open( std::string const & topic_name, std::shared_ptr< dds_subscriber > const & subscriber )
{
    if ( is_open() )
        DDS_THROW( runtime_error, "stream '" + name() + "' is already open" );
    if ( profiles().empty() )
        DDS_THROW( runtime_error, "stream '" + name() + "' has no profiles" );

    //Topics with same name and type can be created multiple times (multiple open() calls) without an error.
    auto image_topic = topics::device::image::create_topic( subscriber->get_participant(), topic_name.c_str() );
    auto md_topic = topics::flexible_msg::create_topic( subscriber->get_participant(), ( topic_name + "/md" ).c_str() );

    //To support automatic streaming (without the need to handle start/stop-streaming commands) the readers are created
    //here and destroyed on close()
    _image_reader = std::make_shared< dds_topic_reader >( image_topic, subscriber );
    _image_reader->on_data_available( [&]() { handle_frames(); } );
    _image_reader->run( dds_topic_reader::qos( eprosima::fastdds::dds::BEST_EFFORT_RELIABILITY_QOS ) );  // no retries

    _metadata_reader = std::make_shared< dds_topic_reader >( md_topic, subscriber );
    _metadata_reader->on_data_available( [&]() { handle_metadata(); } );
    _metadata_reader->run( dds_topic_reader::qos( eprosima::fastdds::dds::BEST_EFFORT_RELIABILITY_QOS ) );  // no retries
}


void dds_stream::close()
{
    _image_reader->on_data_available( [](){} );
    _image_reader.reset();

    _metadata_reader->on_data_available( [](){} );
    _metadata_reader.reset();
}


void dds_stream::start_streaming( on_data_available_callback cb )
{
    if ( !is_open() )
        DDS_THROW( runtime_error, "stream '" + name() + "' is not open" );
    if( is_streaming() )
        DDS_THROW( runtime_error, "stream '" + name() + "' is already streaming" );

    _syncer->reset( cb );
}


void dds_stream::handle_frames()
{
    topics::device::image frame;
    eprosima::fastdds::dds::SampleInfo info;
    while ( _image_reader && topics::device::image::take_next( *_image_reader, &frame, &info ) )
    {
        if ( !frame.is_valid() )
            continue;

        _syncer->enqueue_image( std::move( frame ) );
    }
}


void dds_stream::handle_metadata()
{
    topics::flexible_msg metadata;
    eprosima::fastdds::dds::SampleInfo info;
    while( _image_reader && topics::flexible_msg::take_next( *_metadata_reader, &metadata, &info ) )
    {
        if( !metadata.is_valid() )
            continue;

        _syncer->enqueue_metadata( std::move( metadata ) );
    }
}


void dds_stream::stop_streaming()
{
    if ( !is_streaming() )
        DDS_THROW( runtime_error, "stream '" + name() + "' is not streaming" );

    _syncer->reset( nullptr ); // Delete callback
}


std::shared_ptr< dds_topic > const & dds_stream::get_topic() const
{
    DDS_THROW( runtime_error, "stream '" + name() + "' must be open to get_topic()" );
}


dds_video_stream::dds_video_stream( std::string const & stream_name, std::string const & sensor_name )
    : super( stream_name, sensor_name )
    , _impl( std::make_shared< dds_video_stream::impl >() )
{
}


dds_depth_stream::dds_depth_stream( std::string const & stream_name, std::string const & sensor_name )
    : super( stream_name, sensor_name )
{
}


dds_ir_stream::dds_ir_stream( std::string const & stream_name, std::string const & sensor_name )
    : super( stream_name, sensor_name )
{
}


dds_color_stream::dds_color_stream( std::string const & stream_name, std::string const & sensor_name )
    : super( stream_name, sensor_name )
{
}


dds_fisheye_stream::dds_fisheye_stream( std::string const & stream_name, std::string const & sensor_name )
    : super( stream_name, sensor_name )
{
}


dds_confidence_stream::dds_confidence_stream( std::string const & stream_name, std::string const & sensor_name )
    : super( stream_name, sensor_name )
{
}


dds_motion_stream::dds_motion_stream( std::string const & stream_name, std::string const & sensor_name )
    : super( stream_name, sensor_name )
    , _impl( std::make_shared< dds_motion_stream::impl >() )
{
}


dds_accel_stream::dds_accel_stream( std::string const & stream_name, std::string const & sensor_name )
    : super( stream_name, sensor_name )
{
}


dds_gyro_stream::dds_gyro_stream( std::string const & stream_name, std::string const & sensor_name )
    : super( stream_name, sensor_name )
{
}


dds_pose_stream::dds_pose_stream( std::string const & stream_name, std::string const & sensor_name )
    : super( stream_name, sensor_name )
{
}

} //namespace realdds
