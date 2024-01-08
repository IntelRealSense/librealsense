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
#include <rsutils/string/nocase.h>
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
void to_json( rsutils::json & j, Duration_t const & duration )
{
    if( duration == c_TimeInfinite )
        j = "infinite";
    else if( duration == c_TimeInvalid )
        j = "invalid";
    else
        j = realdds::time_to_double( duration );
}


// Allow j.get< eprosima::fastrtps::Duration_t >();
void from_json( rsutils::json const & j, Duration_t & duration )
{
    if( j.is_string() )
    {
        auto & s = j.string_ref();
        if( rsutils::string::nocase_equal( s, "infinite" ) )
            duration = c_TimeInfinite;
        else if( rsutils::string::nocase_equal( s, "invalid" ) )
            duration = c_TimeInvalid;
        else
            throw rsutils::json::type_error::create( 317, "unknown duration value '" + s + "'", &j );
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


void override_reliability_qos_from_json( eprosima::fastdds::dds::ReliabilityQosPolicy & qos, rsutils::json const & j )
{
    if( j.is_string() )
        qos.kind = reliability_kind_from_string( j.string_ref() );
    else if( j.is_object() )
    {
        if( auto kind_j = j.nested( "kind", &rsutils::json::is_string ) )
            qos.kind = reliability_kind_from_string( kind_j.string_ref() );
    }
}


void override_durability_qos_from_json( eprosima::fastdds::dds::DurabilityQosPolicy & qos, rsutils::json const & j )
{
    if( j.is_string() )
        qos.kind = durability_kind_from_string( j.get_ref< const rsutils::json::string_t & >() );
    else if( j.is_object() )
    {
        if( auto kind_j = j.nested( "kind", &rsutils::json::is_string ) )
            qos.kind = durability_kind_from_string( kind_j.string_ref() );
    }
}


void override_history_qos_from_json( eprosima::fastdds::dds::HistoryQosPolicy & qos, rsutils::json const & j )
{
    if( j.is_number_unsigned() )
        qos.depth = j.get< int32_t >();
    else if( j.is_object() )
    {
        if( auto kind_j = j.nested( "kind", &rsutils::json::is_string ) )
            qos.kind = history_kind_from_string( kind_j.string_ref() );
        j.nested( "depth" ).get_ex( qos.depth );
    }
}


void override_liveliness_qos_from_json( eprosima::fastdds::dds::LivelinessQosPolicy & qos, rsutils::json const & j )
{
    if( j.is_object() )
    {
        if( auto kind = j.nested( "kind" ) )
        {
            if( kind.is_string() )
                qos.kind = liveliness_kind_from_string( kind.string_ref() );
            else
                DDS_THROW( runtime_error, "liveliness kind not a string: " << kind );
        }

        if( auto lease = j.nested( "lease-duration" ) )
        {
            if( lease.is_null() )
                qos.lease_duration = eprosima::fastdds::dds::LivelinessQosPolicy().lease_duration;
            else
                lease.get_to( qos.lease_duration );
        }

        if( auto announce = j.nested( "announcement-period" ) )
        {
            if( announce.is_null() )
                qos.announcement_period = eprosima::fastdds::dds::LivelinessQosPolicy().announcement_period;
            else
                announce.get_to( qos.announcement_period );
        }
    }
}


void override_data_sharing_qos_from_json( eprosima::fastdds::dds::DataSharingQosPolicy & qos, rsutils::json const & j )
{
    if( j.is_boolean() )
    {
        if( j.get< bool >() )
            qos.automatic();
        else
            qos.off();
    }
    else if( j.exists() )
    {
        DDS_THROW( runtime_error, "data-sharing must be a boolean (off/automatic); got " << j );
    }
}


void override_endpoint_qos_from_json( eprosima::fastdds::dds::RTPSEndpointQos & qos, rsutils::json const & j )
{
    if( j.is_object() )
    {
        if( auto policy_j = j.nested( "history-memory-policy", &rsutils::json::is_string ) )
            qos.history_memory_policy = history_memory_policy_from_string( policy_j.string_ref() );
    }
}


static bool parse_ip_list( rsutils::json const & j, std::string const & key, std::vector< std::string > * output )
{
    if( auto whitelist_j = j.nested( key ) )
    {
        if( ! whitelist_j.is_array() )
            return false;
        for( auto & ip : whitelist_j )
        {
            if( ! ip.is_string() )
                return false;
            if( output )
                output->push_back( ip.string_ref() );
        }
    }
    return true;
}


static void override_udp_settings( eprosima::fastdds::rtps::UDPTransportDescriptor & udp, rsutils::json const & j )
{
    j.nested( "send-buffer-size" ).get_ex( udp.sendBufferSize );
    j.nested( "receive-buffer-size" ).get_ex( udp.receiveBufferSize );
    if( ! parse_ip_list( j, "whitelist", &udp.interfaceWhiteList ) )
        LOG_WARNING( "invalid UDP whitelist in settings" );
}


void override_participant_qos_from_json( eprosima::fastdds::dds::DomainParticipantQos & qos, rsutils::json const & j )
{
    if( ! j.is_object() )
        return;
    j.nested( "participant-id" ).get_ex( qos.wire_protocol().participant_id );
    j.nested( "lease-duration" ).get_ex( qos.wire_protocol().builtin.discovery_config.leaseDuration );

    j.nested( "use-builtin-transports" ).get_ex( qos.transport().use_builtin_transports );
    if( auto udp_j = j.nested( "udp" ) )
    {
        for( auto t : qos.transport().user_transports )
            if( auto udp_t = std::dynamic_pointer_cast< eprosima::fastdds::rtps::UDPTransportDescriptor >( t ) )
            {
                override_udp_settings( *udp_t, udp_j );
                break;
            }
    }
}


}  // namespace realdds
