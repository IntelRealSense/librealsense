// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#include <realdds/dds-metadata-syncer.h>
#include <realdds/dds-utilities.h>


namespace realdds {


void dds_metadata_syncer::enqueue_frame( id_type id, frame_holder && frame )
{
    std::lock_guard< std::mutex > lock( _queues_lock );
    while( _frame_queue.size() >= max_frame_queue_size )
        handle_frame_without_metadata();
    LOG_DEBUG( "enqueueing frame " << id );
    _frame_queue.push_back( synced_frame{ id, std::move( frame ) } );
    search_for_match();  // Call under lock
}


void dds_metadata_syncer::enqueue_metadata( id_type id, nlohmann::json && md )
{
    std::lock_guard< std::mutex > lock( _queues_lock );
    while( _metadata_queue.size() >= max_md_queue_size )
    {
        LOG_DEBUG( "throwing away metadata: " << _metadata_queue.front()._md.dump() );
        _metadata_queue.pop_front();  // Throw oldest
    }

    LOG_DEBUG( "enqueueing metadata: " << md.dump() );
    _metadata_queue.push_back( synced_metadata{ id, std::move( md ) } );
    search_for_match();  // Call under lock
}


void dds_metadata_syncer::search_for_match()
{
    // Wait for frame + metadata set
    if( _frame_queue.empty() )
        return;

    // We're looking for metadata with the same ID as the next frame
    auto const frame_id = _frame_queue.front()._sync_id;

    // Throw away any old metadata (with ID < the frame) since the frame ID will keep increasing
    while( ! _metadata_queue.empty() )
    {
        auto const md_id = _metadata_queue.front()._sync_id;
        if( frame_id == md_id )
        {
            handle_match();
            break;
        }
        if( frame_id < md_id )
        {
            // newer metadata: keep it, wait for a frame to arrive
            break;
        }
        // Metadata without frame, remove it from queue and check next
        LOG_DEBUG( "throwing away metadata " << md_id << " looking for frame " << frame_id << " match" );
        _metadata_queue.pop_front();
    }
}


void dds_metadata_syncer::handle_match()
{
    synced_frame & synced = _frame_queue.front();
    auto & fh = synced._frame;

    nlohmann::json md = std::move( _metadata_queue.front()._md );
    _metadata_queue.pop_front();

    LOG_DEBUG( "frame " << synced._sync_id << " ready with metadata " << md.dump() );
    if( _on_frame_ready )
        _on_frame_ready( std::move( fh ), std::move( md ) );

    _frame_queue.pop_front();
}


void dds_metadata_syncer::handle_frame_without_metadata()
{
    synced_frame & synced = _frame_queue.front();
    auto & fh = synced._frame;

    nlohmann::json md;
    LOG_DEBUG( "frame " << synced._sync_id << " ready without metadata" );
    if( _on_frame_ready )
        _on_frame_ready( std::move( fh ), std::move( md ) );

    _frame_queue.pop_front();
}


}  // namespace realdds
