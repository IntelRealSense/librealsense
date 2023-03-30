// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#include <realdds/dds-metadata-syncer.h>
#include <realdds/dds-utilities.h>


namespace realdds {


void dds_metadata_syncer::enqueue_frame( id_type id, frame_holder && frame )
{
    std::lock_guard< std::mutex > lock( _queues_lock );
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
    // Wait for frame + metadata set, but if metadata is lost pass frame to the user anyway
    if( _frame_queue.empty() || (_metadata_queue.empty() && _frame_queue.size() < max_frame_queue_size) )
        return;

    // Sync using frame ID. Frame IDs are assumed to be increasing with time
    auto frame_id = _frame_queue.front()._sync_id;
    auto md_id = _metadata_queue.empty() ? frame_id + 1  // If no metadata force call to handle_frame_no_metadata
        : _metadata_queue.front()._sync_id;

    while( frame_id > md_id && _metadata_queue.size() > 1 )
    {
        // Metadata without frame, remove it from queue and check next
        LOG_DEBUG( "throwing away metadata (looking for id " << frame_id << "): " << _metadata_queue.front()._md.dump() );
        _metadata_queue.pop_front();
        md_id = _metadata_queue.front()._sync_id;
    }

    if( frame_id == md_id )
        handle_match();
    else
        handle_frame_without_metadata();
}


void dds_metadata_syncer::handle_match()
{
    auto & fh = _frame_queue.front()._frame;

    nlohmann::json md;
    if( !_metadata_queue.empty() )
    {
        md = std::move( _metadata_queue.front()._md );
        _metadata_queue.pop_front();
    }

    LOG_DEBUG( "frame " << _frame_queue.front()._sync_id << " ready with metadata " << md.dump() );
    if( _on_frame_ready )
        _on_frame_ready( std::move( fh ), std::move( md ) );

    _frame_queue.pop_front();
}


void dds_metadata_syncer::handle_frame_without_metadata()
{
    auto & fh = _frame_queue.front()._frame;

    nlohmann::json md;
    LOG_DEBUG( "frame " << _frame_queue.front()._sync_id << " ready without metadata" );
    if( _on_frame_ready )
        _on_frame_ready( std::move( fh ), std::move( md ) );

    _frame_queue.pop_front();
}


}  // namespace realdds
