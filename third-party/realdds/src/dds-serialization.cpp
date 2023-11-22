// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#include <realdds/dds-serialization.h>
#include <realdds/dds-utilities.h>
#include <realdds/dds-time.h>

#include <fastdds/rtps/writer/WriterDiscoveryInfo.h>
#include <fastdds/rtps/reader/ReaderDiscoveryInfo.h>

#include <fastdds/dds/domain/qos/DomainParticipantQos.hpp>
#include <fastdds/rtps/transport/UDPTransportDescriptor.h>

#include <rsutils/string/from.h>
#include <rsutils/json.h>


namespace eprosima {
namespace fastdds {
namespace dds {


std::ostream & operator<<( std::ostream & os, ReliabilityQosPolicyKind kind )
{
    switch( kind )
    {
    case eprosima::fastdds::dds::BEST_EFFORT_RELIABILITY_QOS:
        return os << "best-effort";
    case eprosima::fastdds::dds::RELIABLE_RELIABILITY_QOS:
        return os << "reliable";
    }
    return os << (int)kind;
}


std::ostream & operator<<( std::ostream & os, ReliabilityQosPolicy const & qos )
{
    return os << qos.kind;
}


std::ostream & operator<<( std::ostream & os, DurabilityQosPolicyKind kind )
{
    switch( kind )
    {
    case eprosima::fastdds::dds::VOLATILE_DURABILITY_QOS:
        return os << "volatile";
    case eprosima::fastdds::dds::TRANSIENT_LOCAL_DURABILITY_QOS:
        return os << "transient-local";
    case eprosima::fastdds::dds::TRANSIENT_DURABILITY_QOS:
        return os << "transient";
    case eprosima::fastdds::dds::PERSISTENT_DURABILITY_QOS:
        return os << "persistent";
    }
    return os << (int)kind;
}


std::ostream & operator<<( std::ostream & os, DurabilityQosPolicy const & qos )
{
    return os << qos.kind;
}


std::ostream & operator<<( std::ostream & os, HistoryQosPolicy const & qos )
{
    return os;
}


std::ostream & operator<<( std::ostream & os, LivelinessQosPolicyKind kind )
{
    switch( kind )
    {
    case eprosima::fastdds::dds::AUTOMATIC_LIVELINESS_QOS:
        return os << "automatic";
    case eprosima::fastdds::dds::MANUAL_BY_PARTICIPANT_LIVELINESS_QOS:
        return os << "by-participant";
    case eprosima::fastdds::dds::MANUAL_BY_TOPIC_LIVELINESS_QOS:
        return os << "by-topic";
    }
    return os << (int) kind;
}


std::ostream & operator<<( std::ostream & os, LivelinessQosPolicy const & qos )
{
    os << qos.kind;
    if( qos.lease_duration != eprosima::fastrtps::c_TimeInfinite )
        os << "/" << rsutils::string::from( qos.lease_duration.to_ns() / 1e9 ) << 's';
    if( qos.kind == eprosima::fastdds::dds::AUTOMATIC_LIVELINESS_QOS
        && qos.announcement_period != eprosima::fastrtps::c_TimeInfinite )
        os << "/" << rsutils::string::from( qos.announcement_period.to_ns() / 1e9 ) << 's';
    return os;
}


std::ostream & operator<<( std::ostream & os, DataSharingQosPolicy const & qos )
{
    return os;
}


std::ostream & operator<<( std::ostream & os, RTPSEndpointQos const & qos )
{
    return os;
}


}  // namespace dds
}  // namespace fastdds
}  // namespace eprosima


namespace eprosima {
namespace fastrtps {


// Allow j["key"] = qos.lease_duration;
void to_json( nlohmann::json & j, Duration_t const & duration )
{
    if( duration == c_TimeInfinite )
        j = "infinite";
    else if( duration == c_TimeInvalid )
        j = "invalid";
    else
        j = realdds::time_to_double( duration );
}


// Allow j.get< eprosima::fastrtps::Duration_t >();
void from_json( nlohmann::json const & j, Duration_t & duration )
{
    if( j.is_string() )
    {
        auto & s = rsutils::json::string_ref( j );
        // TODO switch to rsutils::string::nocaase_equal()
        if( s == "infinite" )
            duration = c_TimeInfinite;
        else if( s == "invalid" )
            duration = c_TimeInvalid;
        else
            throw nlohmann::json::type_error::create( 317, "unknown duration value '" + s + "'", &j );
    }
    else
        duration = realdds::dds_time( j.get< double >() );
}


namespace rtps {


std::ostream & operator<<( std::ostream & os, WriterProxyData const & info )
{
    os << "'" << info.topicName() << "' [";
    os << /*"type " <<*/ info.typeName();
    os << /*" reliability"*/ " " << info.m_qos.m_reliability;
    if( ! ( info.m_qos.m_durability == eprosima::fastdds::dds::DurabilityQosPolicy() ) )
        os << /*" durability"*/ " " << info.m_qos.m_durability;
    if( ! ( info.m_qos.m_liveliness == eprosima::fastdds::dds::LivelinessQosPolicy() ) )
        os << " liveliness" " " << info.m_qos.m_liveliness;
    os << "]";
    return os;
}


std::ostream & operator<<( std::ostream & os, ReaderProxyData const & info )
{
    os << "'" << info.topicName() << "' [";
    os << /*"type " <<*/ info.typeName();
    os << /*" reliability"*/ " " << info.m_qos.m_reliability;
    if( !(info.m_qos.m_durability == eprosima::fastdds::dds::DurabilityQosPolicy()) )
        os << /*" durability"*/ " " << info.m_qos.m_durability;
    os << "]";
    return os;
}


}  // namespace rtps
}  // namespace fastrtps
}  // namespace eprosima


namespace realdds {


eprosima::fastdds::dds::ReliabilityQosPolicyKind reliability_kind_from_string( std::string const & s )
{
    if( s == "best-effort" )
        return eprosima::fastdds::dds::BEST_EFFORT_RELIABILITY_QOS;
    if( s == "reliable" )
        return eprosima::fastdds::dds::RELIABLE_RELIABILITY_QOS;
    DDS_THROW( runtime_error, "invalid reliability kind '" << s << "'" );
}


eprosima::fastdds::dds::DurabilityQosPolicyKind durability_kind_from_string( std::string const & s )
{
    if( s == "volatile" )
        return eprosima::fastdds::dds::VOLATILE_DURABILITY_QOS;
    if( s == "transient-local" )
        return eprosima::fastdds::dds::TRANSIENT_LOCAL_DURABILITY_QOS;
    if( s == "transient" )
        return eprosima::fastdds::dds::TRANSIENT_DURABILITY_QOS;
    if( s == "persistent" )
        return eprosima::fastdds::dds::PERSISTENT_DURABILITY_QOS;
    DDS_THROW( runtime_error, "invalid durability kind '" << s << "'" );
}


eprosima::fastdds::dds::HistoryQosPolicyKind history_kind_from_string( std::string const & s )
{
    if( s == "keep-last" )
        return eprosima::fastdds::dds::KEEP_LAST_HISTORY_QOS;
    if( s == "keep-all" )
        return eprosima::fastdds::dds::KEEP_ALL_HISTORY_QOS;
    DDS_THROW( runtime_error, "invalid history kind '" << s << "'" );
}


eprosima::fastdds::dds::LivelinessQosPolicyKind liveliness_kind_from_string( std::string const & s )
{
    if( s == "automatic" )
        return eprosima::fastdds::dds::AUTOMATIC_LIVELINESS_QOS;
    if( s == "by-participant" )
        return eprosima::fastdds::dds::MANUAL_BY_PARTICIPANT_LIVELINESS_QOS;
    if( s == "by-topic" )
        return eprosima::fastdds::dds::MANUAL_BY_TOPIC_LIVELINESS_QOS;
    DDS_THROW( runtime_error, "invalid liveliness kind '" << s << "'" );
}


eprosima::fastrtps::rtps::MemoryManagementPolicy_t history_memory_policy_from_string( std::string const & s )
{
    if( s == "preallocated" )
        return eprosima::fastrtps::rtps::PREALLOCATED_MEMORY_MODE;
    if( s == "preallocated-with-realloc" )
        return eprosima::fastrtps::rtps::PREALLOCATED_WITH_REALLOC_MEMORY_MODE;
    if( s == "dynamic-reserve" )
        return eprosima::fastrtps::rtps::DYNAMIC_RESERVE_MEMORY_MODE;
    if( s == "dynamic-reusable" )
        return eprosima::fastrtps::rtps::DYNAMIC_REUSABLE_MEMORY_MODE;
    DDS_THROW( runtime_error, "invalid history memory policy '" << s << "'" );
}


void override_reliability_qos_from_json( eprosima::fastdds::dds::ReliabilityQosPolicy & qos, nlohmann::json const & j )
{
    if( j.is_null() )
        return;
    if( j.is_string() )
        qos.kind = reliability_kind_from_string( rsutils::json::string_ref( j ) );
    else if( j.is_object() )
    {
        std::string kind_str;
        if( rsutils::json::get_ex( j, "kind", &kind_str ) )
            qos.kind = reliability_kind_from_string( kind_str );
    }
}


void override_durability_qos_from_json( eprosima::fastdds::dds::DurabilityQosPolicy & qos, nlohmann::json const & j )
{
    if( j.is_null() )
        return;
    if( j.is_string() )
        qos.kind = durability_kind_from_string( j.get_ref< const nlohmann::json::string_t & >() );
    else if( j.is_object() )
    {
        std::string kind_str;
        if( rsutils::json::get_ex( j, "kind", &kind_str ) )
            qos.kind = durability_kind_from_string( kind_str );
    }
}


void override_history_qos_from_json( eprosima::fastdds::dds::HistoryQosPolicy & qos, nlohmann::json const & j )
{
    if( j.is_null() )
        return;
    if( j.is_number_unsigned() )
        qos.depth = rsutils::json::value< int32_t >( j );
    else if( j.is_object() )
    {
        std::string kind_str;
        if( rsutils::json::get_ex( j, "kind", &kind_str ) )
            qos.kind = history_kind_from_string( kind_str );
        rsutils::json::get_ex( j, "depth", &qos.depth );
    }
}


void override_liveliness_qos_from_json( eprosima::fastdds::dds::LivelinessQosPolicy & qos, nlohmann::json const & j )
{
    if( j.is_object() )
    {
        if( auto kind = rsutils::json::nested( j, "kind" ) )
        {
            if( kind->is_string() )
                qos.kind = liveliness_kind_from_string( rsutils::json::string_ref( kind ) );
            else
                DDS_THROW( runtime_error, "liveliness kind not a string: " << kind );
        }

        if( auto lease = rsutils::json::nested( j, "lease-duration" ) )
        {
            if( lease->is_null() )
                qos.lease_duration = eprosima::fastdds::dds::LivelinessQosPolicy().lease_duration;
            else
                lease->get_to( qos.lease_duration );
        }

        if( auto announce = rsutils::json::nested( j, "announcement-period" ) )
        {
            if( announce->is_null() )
                qos.announcement_period = eprosima::fastdds::dds::LivelinessQosPolicy().announcement_period;
            else
                announce->get_to( qos.announcement_period );
        }
    }
}


void override_data_sharing_qos_from_json( eprosima::fastdds::dds::DataSharingQosPolicy & qos, nlohmann::json const & j )
{
    if( j.is_boolean() )
    {
        if( rsutils::json::value< bool >( j ) )
            qos.automatic();
        else
            qos.off();
    }
    else if( ! j.is_null() )
    {
        DDS_THROW( runtime_error, "data-sharing must be a boolean (off/automatic); got " << j );
    }
}


void override_endpoint_qos_from_json( eprosima::fastdds::dds::RTPSEndpointQos & qos, nlohmann::json const & j )
{
    if( j.is_null() )
        return;
    if( j.is_object() )
    {
        std::string policy_str;
        if( rsutils::json::get_ex( j, "history-memory-policy", &policy_str ) )
            qos.history_memory_policy = history_memory_policy_from_string( policy_str );
    }
}


void override_participant_qos_from_json( eprosima::fastdds::dds::DomainParticipantQos & qos, nlohmann::json const & j )
{
    if( ! j.is_object() )
        return;
    rsutils::json::get_ex( j, "participant-id", &qos.wire_protocol().participant_id );
    rsutils::json::get_ex( j, "lease-duration", &qos.wire_protocol().builtin.discovery_config.leaseDuration );

    rsutils::json::get_ex( j, "use-builtin-transports", &qos.transport().use_builtin_transports );
    if( auto udp_j = rsutils::json::nested( j, "udp" ) )
    {
        for( auto t : qos.transport().user_transports )
            if( auto udp_t = std::dynamic_pointer_cast< eprosima::fastdds::rtps::UDPTransportDescriptor >( t ) )
            {
                rsutils::json::get_ex( udp_j, "send-buffer-size", &udp_t->sendBufferSize );
                rsutils::json::get_ex( udp_j, "receive-buffer-size", &udp_t->receiveBufferSize );
                if( auto whitelist_j = rsutils::json::nested( udp_j, "whitelist" ) )
                {
                    if( ! whitelist_j->is_array() )
                        LOG_WARNING( "UDP whitelist in settings should be an array" );
                    else
                        for( auto & ip : whitelist_j.get() )
                        {
                            if( ip.is_string() )
                                udp_t->interfaceWhiteList.push_back( rsutils::json::string_ref( ip ) );
                        }
                }
                break;
            }
    }
}


}  // namespace realdds
