// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.
#pragma once

#include <fastdds/dds/core/policy/QosPolicies.hpp>

#include <rsutils/json-fwd.h>
#include <iosfwd>


namespace eprosima {
namespace fastdds {
namespace dds {
std::ostream & operator<<( std::ostream &, ReliabilityQosPolicyKind );
std::ostream & operator<<( std::ostream &, ReliabilityQosPolicy const & );
std::ostream & operator<<( std::ostream &, DurabilityQosPolicyKind );
std::ostream & operator<<( std::ostream &, DurabilityQosPolicy const & );
std::ostream & operator<<( std::ostream &, HistoryQosPolicy const & );
std::ostream & operator<<( std::ostream &, LivelinessQosPolicyKind );
std::ostream & operator<<( std::ostream &, LivelinessQosPolicy const & );
std::ostream & operator<<( std::ostream &, DataSharingQosPolicy const & );
std::ostream & operator<<( std::ostream &, RTPSEndpointQos const & );

class DomainParticipantQos;
}  // namespace dds
}  // namespace fastdds
}  // namespace eprosima

namespace eprosima {
namespace fastrtps {

// Allow j["key"] = qos.lease_duration;
void to_json( rsutils::json &, Duration_t const & );
// Allow j.get< eprosima::fastrtps::Duration_t >();
void from_json( rsutils::json const &, Duration_t & );

namespace rtps {
std::ostream & operator<<( std::ostream &, class WriterProxyData const & );
std::ostream & operator<<( std::ostream &, class ReaderProxyData const & );
}  // namespace rtps

}  // namespace fastrtps
}  // namespace eprosima


namespace realdds {


eprosima::fastdds::dds::ReliabilityQosPolicyKind reliability_kind_from_string( std::string const & );
eprosima::fastdds::dds::DurabilityQosPolicyKind durability_kind_from_string( std::string const & );
eprosima::fastdds::dds::HistoryQosPolicyKind history_kind_from_string( std::string const & );
eprosima::fastdds::dds::LivelinessQosPolicyKind liveliness_kind_from_string( std::string const & );
eprosima::fastrtps::rtps::MemoryManagementPolicy_t history_memory_policy_from_string( std::string const & );

// Override QoS reliability from a JSON source.
// The JSON can be a simple string indicating simple reliability kind:
//      "reliability": "best-effort"  // <-- the JSON is the "best-effort" value
// If an object, more detail is possible and it's possible to extend:
//      "reliability": {
//          "kind": "reliable"
//          }
//
void override_reliability_qos_from_json( eprosima::fastdds::dds::ReliabilityQosPolicy & qos, rsutils::json const & );

// Override QoS durability from a JSON source.
// The JSON can be a simple string indicating simple durability kind:
//      "durability": "volatile"  // <-- the JSON is the "volatile" value
// If an object, more detail is possible and it's possible to extend:
//      "durability": {
//          "kind": "persistent"
//          }
//
void override_durability_qos_from_json( eprosima::fastdds::dds::DurabilityQosPolicy & qos, rsutils::json const & );

// Override QoS history from a JSON source.
// The JSON can be a simple unsigned integer indicating simple history depth:
//      "history": 10  // <-- the JSON is the 10 value
// If an object, more detail is possible and it's possible to extend:
//      "history": {
//          "kind": "keep-last",
//          "depth": 20
//          }
//
void override_history_qos_from_json( eprosima::fastdds::dds::HistoryQosPolicy & qos, rsutils::json const & );


// Override QoS liveliness from a JSON source.
// Liveliness settings are an object
//      {
//          "kind": "automatic",
//          "lease-duration": 5,      // seconds
//          "announcement-period": 3  // seconds
//      }
//
void override_liveliness_qos_from_json( eprosima::fastdds::dds::LivelinessQosPolicy & qos, rsutils::json const & );


// Override QoS data-sharing from a JSON source.
// The JSON can be a simple boolean indicating off or automatic mode:
//      "data-sharing": true  // <-- the JSON is the true value
//
void override_data_sharing_qos_from_json( eprosima::fastdds::dds::DataSharingQosPolicy & qos, rsutils::json const & );

// Override QoS endpoint from a JSON source.
// The JSON is an object:
//      {
//          "history-memory-policy": "preallocated-with-realloc"
//      }
//
void override_endpoint_qos_from_json( eprosima::fastdds::dds::RTPSEndpointQos & qos, rsutils::json const & );


// Override participant QoS from a JSON source.
// The JSON is an object:
//      {
//          "participant-id": -1,
//          "lease-duration": 10,  // seconds
//      }
//
void override_participant_qos_from_json( eprosima::fastdds::dds::DomainParticipantQos & qos, rsutils::json const & );


}  // namespace realdds
