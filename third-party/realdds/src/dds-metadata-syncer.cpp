// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#include <realdds/dds-metadata-syncer.h>
#include <realdds/dds-utilities.h>


namespace realdds {


const size_t dds_metadata_syncer::max_md_queue_size = 8;
const size_t dds_metadata_syncer::max_frame_queue_size = 2;


dds_metadata_syncer::dds_metadata_syncer()
    : _is_alive( std::make_shared< bool >( true ) )
    , _on_frame_release( nullptr )
{
}


dds_metadata_syncer::~dds_metadata_syncer()
{
    _is_alive.reset();

    std::lock_guard< std::mutex > lock( _queues_lock );
    _frame_queue.clear();
    _metadata_queue.clear();
}


void dds_metadata_syncer::enqueue_frame( key_type id, frame_holder && frame )
{
    std::weak_ptr< bool > alive = _is_alive;
    if( ! alive.lock() ) // Check if was destructed by another thread
        return;

    std::unique_lock< std::mutex > lock( _queues_lock );
    // Expect increasing order
    if( ! _frame_queue.empty() && _frame_queue.back().first >= id )
        DDS_THROW( runtime_error, "frame " << id << " cannot be enqueued after " << _frame_queue.back().first );

    // We must push the new one before releasing the lock, else someone else may push theirs ahead of ours
    _frame_queue.push_back( key_frame{ id, std::move( frame ) } );

    while( _frame_queue.size() > max_frame_queue_size )
        if( ! handle_frame_without_metadata( lock ) ) // Lock released and aquired around callbacks, check we are alive
            return;

    search_for_match( lock );
}


void dds_metadata_syncer::enqueue_metadata( key_type id, metadata_type const & md )
{
    std::weak_ptr< bool > alive = _is_alive;
    if( ! alive.lock() )  // Check if was destructed by another thread
        return;

    std::unique_lock< std::mutex > lock( _queues_lock );
    // Expect increasing order
    if( ! _metadata_queue.empty() && _metadata_queue.back().first >= id )
        DDS_THROW( runtime_error, "metadata " << id << " cannot be enqueued after " << _metadata_queue.back().first );

    // We must push the new one before releasing the lock, else someone else may push theirs ahead of ours
    _metadata_queue.push_back( key_metadata{ id, md } );

    while( _metadata_queue.size() > max_md_queue_size )
        if( ! drop_metadata( lock ) ) // Lock released and aquired around callbacks, check we are alive
            return;

    search_for_match( lock );
}


void dds_metadata_syncer::search_for_match( std::unique_lock< std::mutex > & lock )
{
    // Wait for frame + metadata set
    while( ! _frame_queue.empty() && ! _metadata_queue.empty() )
    {
        // We're looking for metadata with the same ID as the next frame
        auto const frame_key = _frame_queue.front().first;
        auto const md_key = _metadata_queue.front().first;

        if( frame_key < md_key )
        {
            // Newer metadata: we can release the frame
            if( ! handle_frame_without_metadata( lock ) )
                return;
        }
        else if( frame_key == md_key )
        {
            if( ! handle_match( lock ) )
                return;
        }
        else
        {
            // Throw away any old metadata (with ID < the frame) since the frame ID will keep increasing
            if( ! drop_metadata( lock ) )
                return;
        }
    }
}


bool dds_metadata_syncer::handle_match( std::unique_lock< std::mutex > & lock )
{
    std::weak_ptr< bool > alive = _is_alive;

    frame_holder fh = std::move( _frame_queue.front().second );
    metadata_type md = std::move( _metadata_queue.front().second );
    _metadata_queue.pop_front();
    _frame_queue.pop_front();

    if( _on_frame_ready )
    {
        lock.unlock();
        _on_frame_ready( std::move( fh ), md );
        if( ! alive.lock() )  // Check if was destructed by another thread during callback
            return false;
        lock.lock();
    }

    return true;
}


bool dds_metadata_syncer::handle_frame_without_metadata( std::unique_lock< std::mutex > & lock )
{
    std::weak_ptr< bool > alive = _is_alive;

    frame_holder fh = std::move( _frame_queue.front().second );
    _frame_queue.pop_front();

    if( _on_frame_ready )
    {
        lock.unlock();
        _on_frame_ready( std::move( fh ), metadata_type() );
        if( ! alive.lock() )  // Check if was destructed by another thread during callback
            return false;
        lock.lock();
    }

    return true;
}


bool dds_metadata_syncer::drop_metadata( std::unique_lock< std::mutex > & lock )
{
    std::weak_ptr< bool > alive = _is_alive;

    auto key = _metadata_queue.front().first;
    auto md = std::move( _metadata_queue.front().second );
    _metadata_queue.pop_front();  // Throw oldest
    if( _on_metadata_dropped )
    {
        lock.unlock();
        _on_metadata_dropped( key, md );
        if( ! alive.lock() )  // Check if was destructed by another thread during callback
            return false;
        lock.lock();
    }

    return true;
}


}  // namespace realdds
