// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once


#include <fastdds/dds/log/Log.hpp>
#include <memory>


namespace realdds {


// Convert FastDDS Log::Entry to EasyLogging log (see ELPP_WRITE_LOG)
#define LOG_DDS_ENTRY( ENTRY, LEVEL, ... )                                                                             \
    do                                                                                                                 \
    {                                                                                                                  \
        char const * filename = ( ENTRY ).context.filename;                                                            \
        char const * func = ( ENTRY ).context.function;                                                                \
        if( ! func )                                                                                                   \
            func = "n/a";                                                                                              \
        if( ! filename )                                                                                               \
            filename = func;                                                                                           \
        el::base::Writer writer( el::Level::LEVEL, filename, ( ENTRY ).context.line, func );                           \
        writer.construct( 1, "librealsense" );                                                                         \
        writer << __VA_ARGS__;                                                                                         \
        writer << " [DDS]";                                                                                            \
        if( ( ENTRY ).context.category )                                                                               \
            writer << "[" << ( ENTRY ).context.category << "]";                                                        \
    }                                                                                                                  \
    while( false )


struct log_consumer : eprosima::fastdds::dds::LogConsumer
{
    static std::unique_ptr< eprosima::fastdds::dds::LogConsumer > create()
    {
        return std::unique_ptr< eprosima::fastdds::dds::LogConsumer >( new log_consumer );
    }

    void Consume( const eprosima::fastdds::dds::Log::Entry & e ) override;

private:
    log_consumer() = default;
};


}  // namespace realdds
