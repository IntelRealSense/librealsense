// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#include <rsutils/json.h>
#include <rsutils/os/executable-name.h>

namespace rsutils {
namespace json {


nlohmann::json const null_json = {};
nlohmann::json const empty_json_string = nlohmann::json::value_type( nlohmann::json::value_t::string );
nlohmann::json const empty_json_object = nlohmann::json::object();


void patch( nlohmann::json & j, nlohmann::json const & patches, std::string const & what )
{
    if( ! patches.is_object() )
    {
        std::string context = what.empty() ? std::string( "patch", 5 ) : what;
        throw std::runtime_error( context + ": expecting an object; got " + patches.dump() );
    }

    try
    {
        j.merge_patch( patches );
    }
    catch( std::exception const & e )
    {
        std::string context = what.empty() ? std::string( "patch", 5 ) : what;
        throw std::runtime_error( "failed to merge " + context + ": " + e.what() );
    }
}


nlohmann::json load_app_settings( nlohmann::json const & global,
                                  std::string const & application,
                                  std::string const & subkey,
                                  std::string const & error_context )
{
    // Take the global subkey settings out of the configuration
    nlohmann::json settings;
    if( auto global_subkey = rsutils::json::nested( global, subkey ) )
        patch( settings, global_subkey, "global " + error_context + '/' + subkey );

    // Patch any application-specific subkey settings
    if( auto application_subkey = rsutils::json::nested( global, application, subkey ) )
        patch( settings, application_subkey, error_context + '/' + application + '/' + subkey );

    return settings;
}


nlohmann::json
load_settings( nlohmann::json const & global, std::string const & subkey, std::string const & error_context )
{
    return load_app_settings( global, rsutils::os::executable_name(), subkey, error_context );
}


}  // namespace json
}  // namespace rsutils
