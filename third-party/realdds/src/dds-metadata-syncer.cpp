// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#include <realdds/dds-metadata-syncer.h>
#include <realdds/dds-utilities.h>


namespace realdds {


void dds_metadata_syncer::enqueue_frame( key_type id, frame_holder && frame )
{
    std::unique_lock< std::mutex > lock( _queues_lock );
    // Expect increasing order
    if( ! _frame_queue.empty() && _frame_queue.back().first >= id )
        DDS_THROW( runtime_error,
                   "frame " + std::to_string( id ) + " cannot be enqueued after "
                       + std::to_string( _frame_queue.back().first ) );
    // We must push the new one before releasing the lock, else someone else may push theirs ahead of ours
    _frame_queue.push_back( key_frame{ id, std::move( frame ) } );
    while( _frame_queue.size() > max_frame_queue_size )
        handle_frame_without_metadata( lock );
    search_for_match( lock );
}


void dds_metadata_syncer::enqueue_metadata( key_type id, metadata_type && md )
{
    std::unique_lock< std::mutex > lock( _queues_lock );
    // Expect increasing order
    if( ! _metadata_queue.empty() && _metadata_queue.back().first >= id )
        DDS_THROW( runtime_error,
                   "metadata " + std::to_string( id ) + " cannot be enqueued after "
                       + std::to_string( _metadata_queue.back().first ) );
    // We must push the new one before releasing the lock, else someone else may push theirs ahead of ours
    _metadata_queue.push_back( key_metadata{ id, std::move( md ) } );
    while( _metadata_queue.size() > max_md_queue_size )
        drop_metadata( lock );
    search_for_match( lock );
}


void dds_metadata_syncer::search_for_match( std::unique_lock< std::mutex > & lock )
{
    // Wait for frame + metadata set
    if( _frame_queue.empty() )
        return;

    // We're looking for metadata with the same ID as the next frame
    auto frame_key = _frame_queue.front().first;

    // Throw away any old metadata (with ID < the frame) since the frame ID will keep increasing
    while( ! _metadata_queue.empty() )
    {
        auto const md_key = _metadata_queue.front().first;
        while( frame_key < md_key )
        {
            // newer metadata: we can release the frame
            handle_frame_without_metadata( lock );
            // and advance to the next, if there is any:
            if( _frame_queue.empty() )
                return;
            frame_key = _frame_queue.front().first;
        }
        if( frame_key == md_key )
        {
            handle_match( lock );
            break;
        }
        // Metadata without frame, remove it from queue and check next
        drop_metadata( lock );
    }
}


void dds_metadata_syncer::handle_match( std::unique_lock< std::mutex > & lock )
{
    frame_holder fh = std::move( _frame_queue.front().second );
    metadata_type md = std::move( _metadata_queue.front().second );
    _metadata_queue.pop_front();
    _frame_queue.pop_front();

    if( _on_frame_ready )
    {
        lock.unlock();
        _on_frame_ready( std::move( fh ), std::move( md ) );
        lock.lock();
    }
}


void dds_metadata_syncer::handle_frame_without_metadata( std::unique_lock< std::mutex > & lock )
{
    frame_holder fh = std::move( _frame_queue.front().second );
    _frame_queue.pop_front();

    if( _on_frame_ready )
    {
        lock.unlock();
        metadata_type md;
        _on_frame_ready( std::move( fh ), std::move( md ) );
        lock.lock();
    }
}


void dds_metadata_syncer::drop_metadata( std::unique_lock< std::mutex > & lock )
{
    auto key = _metadata_queue.front().first;
    auto md = std::move( _metadata_queue.front().second );
    _metadata_queue.pop_front();  // Throw oldest
    if( _on_metadata_dropped )
    {
        lock.unlock();
        _on_metadata_dropped( key, std::move( md ) );
        lock.lock();
    }
}


}  // namespace realdds
