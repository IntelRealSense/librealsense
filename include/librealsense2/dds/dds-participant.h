// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include <memory>
#include <functional>
#include <string>


namespace eprosima {
namespace fastdds {
namespace dds {
    class DomainParticipant;
}  // namespace dds
}  // namespace fastdds
namespace fastrtps {
namespace rtps {
    struct GUID_t;
}  // namespace dds
}  // namespace fastdds
}  // namespace eprosima

namespace librealsense {
namespace dds {


using dds_guid = eprosima::fastrtps::rtps::GUID_t;
typedef int dds_domain_id;


// Encapsulate FastDDS "participant" entity
// Use this class if you wish to create & pass a participant entity without the need to include 'FastDDS' headers
class dds_participant
{
    eprosima::fastdds::dds::DomainParticipant * _participant = nullptr;

public:
    dds_participant() = default;
    dds_participant( const dds_participant & ) = delete;
    ~dds_participant();

    void init( dds_domain_id, std::string const & participant_name );

    void on_writer_added( std::function< void( dds_guid guid, char const * ) > callback )
    {
        _on_writer_added = std::move( callback );
    }
    void on_writer_removed( std::function< void( dds_guid guid ) > callback )
    {
        _on_writer_removed = std::move( callback );
    }

    bool is_valid() const { return ( nullptr != _participant ); }
    bool operator!() const { return ! is_valid(); }

    eprosima::fastdds::dds::DomainParticipant * get() const { return _participant; }
    eprosima::fastdds::dds::DomainParticipant * operator->() const { return get(); }

private:
    struct dds_participant_listener;

    std::shared_ptr< dds_participant_listener > _domain_listener;

    std::function< void( dds_guid, char const * topic_name ) > _on_writer_added;
    std::function< void( dds_guid ) > _on_writer_removed;
};  // class dds_participant


}  // namespace dds
}  // namespace librealsense
