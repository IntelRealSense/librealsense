// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#ifdef BUILD_EASYLOGGINGPP
#include <rsutils/easylogging/easyloggingpp.h>

namespace rsutils {


void configure_elpp_logger( bool enable_debug, std::string const & nested_indent, std::string const & logger_id )
{
    el::Configurations defaultConf;

    el::Logger * logger = el::Loggers::getLogger( logger_id );
    el::Configurations * configs;
    if( logger )
    {
        configs = logger->configurations();
    }
    else
    {
        // First initialization
        defaultConf.setToDefault();
        defaultConf.setGlobally( el::ConfigurationType::ToStandardOutput, "false" );
        defaultConf.set( el::Level::Error, el::ConfigurationType::ToStandardOutput, "true" );
        configs = &defaultConf;
    }

    std::string format = "-%levshort- %datetime{%H:%m:%s.%g} %msg (%fbase:%line [%thread])";
    if( ! nested_indent.empty() )
        format = '[' + nested_indent + "] " + format;
    configs->setGlobally( el::ConfigurationType::Format, format );

    auto enable_str = enable_debug ? "true" : "false";
    configs->set( el::Level::Warning, el::ConfigurationType::ToStandardOutput, enable_str );
    configs->set( el::Level::Info, el::ConfigurationType::ToStandardOutput, enable_str );
    configs->set( el::Level::Debug, el::ConfigurationType::ToStandardOutput, enable_str );

    if( logger )
        logger->reconfigure();
    else
        el::Loggers::reconfigureLogger( logger_id, defaultConf );
}


}  // namespace rsutils

#endif  // BUILD_EASYLOGGINGPP
