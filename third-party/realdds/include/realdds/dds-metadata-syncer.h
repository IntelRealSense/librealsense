// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include <nlohmann/json.hpp>

#include <deque>
#include <memory>
#include <mutex>
#include <functional>


namespace realdds {


// Frame data and metadata are sent as two seperate streams which may need synchronizing and joining together.
// 
// This mechanism takes a generic "frame" (as a void*) and "metadata" (any json) and issues a callback whenever a match
// occurs.
// 
// Note this means:
//     - the callback is only called when a frame/metadata is fed to it (enqueued)
//     - callbacks are called on the same thread as the enqueue
//     - frames may be issued without metadata if none is found
//     - metadata by itself is never issued
//
class dds_metadata_syncer
{
public:
    // We don't want the queue to get large, it means lots of drops and data that we store to (probably) throw later
    static constexpr size_t max_md_queue_size = 8;
    // If a metadata is lost we wait for it until the next frame arrives, causing a small delay but we prefer passing
    // the frame late and without metadata over losing it.
    static constexpr size_t max_frame_queue_size = 2;

    // We synchronize using some abstract "id" used to identify each frame and its metadata. We don't need to know
    // the nature of the id; only that it is increasing in value over time so that, given id1 > id2, then id1
    // happened after id2.
    typedef uint64_t id_type;

    // Likewise, we don't know what the nature of a frame is; we hold it as a pointer to something, with a deleter to
    // ensure some lifetime management...
    typedef std::unique_ptr< void, void ( * )( void * ) > frame_holder;

    // So our callback gets this generic frame and metadata:
    typedef std::function< void( frame_holder &&, nlohmann::json && metadata ) > on_frame_ready;

private:
    struct synced_frame
    {
        id_type _sync_id;
        frame_holder _frame;
    };

    struct synced_metadata
    {
        id_type _sync_id;
        nlohmann::json _md;
    };

    on_frame_ready _on_frame_ready = nullptr;

    std::deque< synced_frame > _frame_queue;
    std::deque< synced_metadata > _metadata_queue;
    std::mutex _queues_lock;

public:
    void enqueue_frame( id_type, frame_holder && );
    void enqueue_metadata( id_type, nlohmann::json && );

    void reset( on_frame_ready cb = nullptr )
    {
        std::lock_guard< std::mutex > lock( _queues_lock );
        _frame_queue.clear();
        _metadata_queue.clear();
        _on_frame_ready = cb;
    }

private:
    // Call these under lock:
    void search_for_match();
    void handle_match();
    void handle_frame_without_metadata();
};


}  // namespace realdds
