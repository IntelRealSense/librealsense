// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

namespace eprosima {
namespace fastdds {
namespace dds {
class DomainParticipant;
}
}  // namespace fastdds
}  // namespace eprosima

class dds_participant_listener;

namespace tools {

// Encapsulate FastDDS "participant" entity
// Use this class if you wish to create & pass a participant entity without the need to include 'FastDDS' headers
class dds_participant
{
public:
    dds_participant( int domain_id, std::string name );
    ~dds_participant();
    eprosima::fastdds::dds::DomainParticipant * get() { return _participant; }

private:
    eprosima::fastdds::dds::DomainParticipant * _participant;
    std::shared_ptr< dds_participant_listener > _domain_listener;
};  // class dds_participant
}  // namespace tools
