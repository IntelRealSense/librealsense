// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <realdds/dds-utilities.h>
#include <realdds/dds-log-consumer.h>
#include <realdds/topics/dds-topic-names.h>

namespace realdds {


std::string get_dds_error( eprosima::fastrtps::types::ReturnCode_t ret_code )
{
    switch( ret_code() )
    {
    case eprosima::fastrtps::types::ReturnCode_t::RETCODE_OK: return "OK";
    case eprosima::fastrtps::types::ReturnCode_t::RETCODE_ERROR: return "ERROR";
    case eprosima::fastrtps::types::ReturnCode_t::RETCODE_UNSUPPORTED: return "UNSUPPORTED";
    case eprosima::fastrtps::types::ReturnCode_t::RETCODE_BAD_PARAMETER: return "BAD_PARAMETER";
    case eprosima::fastrtps::types::ReturnCode_t::RETCODE_PRECONDITION_NOT_MET: return "PRECONDITION_NOT_MET";
    case eprosima::fastrtps::types::ReturnCode_t::RETCODE_OUT_OF_RESOURCES: return "OUT_OF_RESOURCES";
    case eprosima::fastrtps::types::ReturnCode_t::RETCODE_NOT_ENABLED: return "NOT_ENABLED";
    case eprosima::fastrtps::types::ReturnCode_t::RETCODE_IMMUTABLE_POLICY: return "IMMUTABLE_POLICY";
    case eprosima::fastrtps::types::ReturnCode_t::RETCODE_INCONSISTENT_POLICY: return "INCONSISTENT_POLICY";
    case eprosima::fastrtps::types::ReturnCode_t::RETCODE_ALREADY_DELETED: return "ALREADY_DELETED";
    case eprosima::fastrtps::types::ReturnCode_t::RETCODE_TIMEOUT: return "TIMEOUT";
    case eprosima::fastrtps::types::ReturnCode_t::RETCODE_NO_DATA: return "NO_DATA";
    case eprosima::fastrtps::types::ReturnCode_t::RETCODE_ILLEGAL_OPERATION: return "ILLEGAL_OPERATION";
    case eprosima::fastrtps::types::ReturnCode_t::RETCODE_NOT_ALLOWED_BY_SECURITY: return "NOT_ALLOWED_BY_SECURITY";
    }
    std::ostringstream os;
    os << "error " << ret_code();
    return os.str();
}


rsutils::string::slice device_name_from_root( std::string const & topic_root )
{
    auto begin = topic_root.c_str();
    auto end = begin + topic_root.length();
    if( topic_root.length() > topics::ROOT_LEN && topics::SEPARATOR == begin[topics::ROOT_LEN - 1] )
        begin += topics::ROOT_LEN;
    return{ begin, end };
}


void log_consumer::Consume( const eprosima::fastdds::dds::Log::Entry & e )
{
    using eprosima::fastdds::dds::Log;
    switch( e.kind )
    {
    case Log::Kind::Error:
        LOG_DDS_ENTRY( e, Error, e.message );
        break;
    case Log::Kind::Warning:
        LOG_DDS_ENTRY( e, Warning, e.message );
        break;
    case Log::Kind::Info:
        LOG_DDS_ENTRY( e, Info, e.message );
        break;
    }
}


}  // namespace realdds
