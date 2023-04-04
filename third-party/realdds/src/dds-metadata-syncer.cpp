// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#include <realdds/dds-metadata-syncer.h>
#include <realdds/dds-utilities.h>


namespace realdds {


void dds_metadata_syncer::enqueue_frame( key_type id, frame_holder && frame )
{
    std::lock_guard< std::mutex > lock( _queues_lock );
    while( _frame_queue.size() >= max_frame_queue_size )
        handle_frame_without_metadata();
    _frame_queue.push_back( key_frame{ id, std::move( frame ) } );
    search_for_match();  // Call under lock
}


void dds_metadata_syncer::enqueue_metadata( key_type id, metadata_type && md )
{
    std::lock_guard< std::mutex > lock( _queues_lock );
    while( _metadata_queue.size() >= max_md_queue_size )
    {
        if( _on_metadata_dropped )
            _on_metadata_dropped( _metadata_queue.front().first, std::move( _metadata_queue.front().second ) );
        _metadata_queue.pop_front();  // Throw oldest
    }

    _metadata_queue.push_back( key_metadata{ id, std::move( md ) } );
    search_for_match();  // Call under lock
}


void dds_metadata_syncer::search_for_match()
{
    // Wait for frame + metadata set
    if( _frame_queue.empty() )
        return;

    // We're looking for metadata with the same ID as the next frame
    auto const frame_key = _frame_queue.front().first;

    // Throw away any old metadata (with ID < the frame) since the frame ID will keep increasing
    while( ! _metadata_queue.empty() )
    {
        auto const md_key = _metadata_queue.front().first;
        if( frame_key == md_key )
        {
            handle_match();
            break;
        }
        if( frame_key < md_key )
        {
            // newer metadata: keep it, wait for a frame to arrive
            break;
        }
        // Metadata without frame, remove it from queue and check next
        if( _on_metadata_dropped )
            _on_metadata_dropped( _metadata_queue.front().first, std::move( _metadata_queue.front().second ) );
        _metadata_queue.pop_front();
    }
}


void dds_metadata_syncer::handle_match()
{
    key_frame & synced = _frame_queue.front();
    auto & fh = synced.second;

    metadata_type md = std::move( _metadata_queue.front().second );
    _metadata_queue.pop_front();

    if( _on_frame_ready )
        _on_frame_ready( std::move( fh ), std::move( md ) );

    _frame_queue.pop_front();
}


void dds_metadata_syncer::handle_frame_without_metadata()
{
    key_frame & synced = _frame_queue.front();
    auto & fh = synced.second;

    metadata_type md;
    if( _on_frame_ready )
        _on_frame_ready( std::move( fh ), std::move( md ) );

    _frame_queue.pop_front();
}


}  // namespace realdds
