// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include "dds-defines.h"

#include <memory>
#include <functional>
#include <string>
#include <list>


namespace librealsense {
namespace dds {


// Encapsulate FastDDS "participant" entity
// Use this class if you wish to create & pass a participant entity without the need to include 'FastDDS' headers
class dds_participant
{
    eprosima::fastdds::dds::DomainParticipant * _participant = nullptr;

    struct listener_impl;

public:
    dds_participant() = default;
    dds_participant( const dds_participant & ) = delete;
    ~dds_participant();

    // Creates the underlying DDS participant and sets the QoS
    // If need to use callbacks set them before calling init, they may be called before init returns.
    void init( dds_domain_id, std::string const & participant_name );

    bool is_valid() const { return ( nullptr != _participant ); }
    bool operator!() const { return ! is_valid(); }

    eprosima::fastdds::dds::DomainParticipant * get() const { return _participant; }
    eprosima::fastdds::dds::DomainParticipant * operator->() const { return get(); }

    dds_guid const & guid() const;

    // Common utility: a participant is usually used as the base for printing guids...
    std::string print( dds_guid const & guid_to_print ) const;

    class listener
    {
        friend class dds_participant;

        std::function< void( dds_guid, char const* topic_name ) > _on_writer_added;
        std::function< void( dds_guid, char const* topic_name ) > _on_writer_removed;
        std::function< void( dds_guid, char const* topic_name ) > _on_reader_added;
        std::function< void( dds_guid, char const* topic_name ) > _on_reader_removed;
        std::function< void( dds_guid, char const* participant_name ) > _on_participant_added;
        std::function< void( dds_guid, char const* participant_name ) > _on_participant_removed;
        std::function< void( char const* topic_name, eprosima::fastrtps::types::DynamicType_ptr dyn_type ) > _on_type_discovery;

        listener() = default;

    public:
        listener* on_writer_added( std::function< void( dds_guid guid, char const* topic_name ) > callback )
        {
            _on_writer_added = std::move( callback );
            return this;
        }
        listener* on_writer_removed( std::function< void( dds_guid guid, char const* topic_name ) > callback )
        {
            _on_writer_removed = std::move( callback );
            return this;
        }
        listener* on_reader_added( std::function< void( dds_guid guid, char const* topic_name ) > callback )
        {
            _on_reader_added = std::move( callback );
            return this;
        }
        listener* on_reader_removed( std::function< void( dds_guid guid, char const* topic_name ) > callback )
        {
            _on_reader_removed = std::move( callback );
            return this;
        }
        listener* on_participant_added( std::function< void( dds_guid guid, char const* participant_name ) > callback )
        {
            _on_participant_added = std::move( callback );
            return this;
        }
        listener* on_participant_removed( std::function< void( dds_guid guid, char const* participant_name ) > callback )
        {
            _on_participant_removed = std::move( callback );
            return this;
        }
        listener* on_type_discovery( std::function< void( char const* topic_name,
            eprosima::fastrtps::types::DynamicType_ptr dyn_type ) > callback )
        {
            _on_type_discovery = std::move( callback );
            return this;
        }
    };

    // Create an instance that gets callbacks and returns a shared_ptr. When the shared_ptr is destroyed the
    // callbacks to it will automatically stop.
    //
    // The shared_ptr must therefore be assigned and kept alive.
    // The out parameter is to enable syntax like this:
    //     _participant.create_listener( &_listener )->on_writer_removed( ... );
    // rather than:
    //     _listener = _participant.create_listener();
    //     _listener->on_writer_removed( ... );
    // The latter is still possible.
    //
    std::shared_ptr< listener > create_listener( std::shared_ptr< listener > * out = nullptr )
    {
        std::shared_ptr< listener > l( new listener );
        std::weak_ptr< listener > wl( l );
        _listeners.emplace_back( wl );
        if( out )
            *out = l;
        return l;
    }

private:
    std::list< std::weak_ptr< listener > > _listeners;
    std::shared_ptr< listener_impl > _domain_listener;

    void on_writer_added( dds_guid, char const * topic_name );
    void on_writer_removed( dds_guid, char const * topic_name );
    void on_reader_added( dds_guid, char const * topic_name );
    void on_reader_removed( dds_guid, char const * topic_name );
    void on_participant_added( dds_guid, char const * participant_name );
    void on_participant_removed( dds_guid, char const * participant_name );
    void on_type_discovery( char const * topic_name, eprosima::fastrtps::types::DynamicType_ptr dyn_type );
};  // class dds_participant


}  // namespace dds
}  // namespace librealsense
