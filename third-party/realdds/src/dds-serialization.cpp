// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#include <realdds/dds-serialization.h>
#include <realdds/dds-utilities.h>

#include <fastdds/rtps/writer/WriterDiscoveryInfo.h>
#include <fastdds/rtps/reader/ReaderDiscoveryInfo.h>

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
namespace rtps {


std::ostream & operator<<( std::ostream & os, WriterProxyData const & info )
{
    os << "'" << info.topicName() << "' [";
    os << /*"type " <<*/ info.typeName();
    os << /*" reliability"*/ " " << info.m_qos.m_reliability;
    if( ! ( info.m_qos.m_durability == eprosima::fastdds::dds::DurabilityQosPolicy() ) )
        os << /*" durability"*/ " " << info.m_qos.m_durability;
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


void override_data_sharing_qos_from_json( eprosima::fastdds::dds::DataSharingQosPolicy & qos, nlohmann::json const & j )
{
    if( j.is_null() )
        return;
    if( j.is_boolean() )
    {
        if( rsutils::json::value< bool >( j ) )
            qos.automatic();
        else
            qos.off();
    }
    else
    {
        DDS_THROW( runtime_error, "data-sharing must be a boolean (off/automatic)" );
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


}  // namespace realdds
