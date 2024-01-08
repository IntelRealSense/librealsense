// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.
#pragma once

#include <rsutils/json.h>

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
// A few assumptions are made:
//     - metadata and frames arrive from different threads, so can happen concurrently
//     - frames are enqueued in strictly increasing order (keys) from the same thread
//          - else no guarantee is made to callback ordering!
//     - metadata is likely to arrive first because the messages are much smaller
//
class dds_metadata_syncer
{
public:
    // We don't want the queue to get large, it means lots of drops and data that we store to (probably) throw later
    static const size_t max_md_queue_size;
    // If a metadata is lost we wait for it until the next frame arrives, causing a small delay but we prefer passing
    // the frame late and without metadata over losing it.
    static const size_t max_frame_queue_size;

    // We synchronize using some abstract "key" used to identify each frame and its metadata. We don't need to know
    // the nature of the key; only that it is increasing in value over time so that, given key1 > key2, then key1
    // happened after key2.
    typedef uint64_t key_type;

    // Likewise, we don't know what the nature of a frame is; we hold it as a pointer to something, with a deleter to
    // ensure some lifetime management...
    typedef void frame_type;

    // NOTE: this is not std::function<>! unique_ptr does not accept any lambda but rather a function pointer.
    typedef void ( *on_frame_release_callback )( frame_type * );
    typedef std::unique_ptr< frame_type, on_frame_release_callback > frame_holder;

    // Metadata is intended to be JSON
    typedef std::shared_ptr< const rsutils::json > metadata_type;

    // So our main callback gets this generic frame and metadata:
    typedef std::function< void( frame_holder &&, metadata_type const & metadata ) > on_frame_ready_callback;

    // And we provide other callbacks, for control, testing, etc.
    typedef std::function< void( key_type, metadata_type const & ) > on_metadata_dropped_callback;

private:
    using key_frame = std::pair< key_type, frame_holder >;
    using key_metadata = std::pair< key_type, metadata_type >;

    std::deque< key_frame > _frame_queue;
    std::deque< key_metadata > _metadata_queue;
    std::mutex _queues_lock;

    on_frame_release_callback _on_frame_release;
    on_frame_ready_callback _on_frame_ready;
    on_metadata_dropped_callback _on_metadata_dropped;

    std::shared_ptr< bool > _is_alive; // Ensures object can be accessed

public:
    dds_metadata_syncer();
    virtual ~dds_metadata_syncer();

    void enqueue_frame( key_type, frame_holder && );
    void enqueue_metadata( key_type, metadata_type const & );

    void on_frame_release( on_frame_release_callback cb ) { _on_frame_release = cb; }
    void on_frame_ready( on_frame_ready_callback cb ) { _on_frame_ready = cb; }
    void on_metadata_dropped( on_metadata_dropped_callback cb ) { _on_metadata_dropped = cb; }

    // Helper to create frame_holder
    template< class Frame >
    inline frame_holder hold( Frame * frame ) const
    {
        return frame_holder( frame, _on_frame_release );
    }

private:
    // Call these under lock:
    void search_for_match( std::unique_lock< std::mutex > & );
    bool handle_match( std::unique_lock< std::mutex > & );
    bool handle_frame_without_metadata( std::unique_lock< std::mutex > & );
    bool drop_metadata( std::unique_lock< std::mutex > & );
};


}  // namespace realdds
