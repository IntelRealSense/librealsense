// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#include <rsutils/json.h>
#include <rsutils/json-config.h>
#include <rsutils/os/executable-name.h>

#include <fstream>


namespace rsutils {


json const null_json = {};
json const missing_json = json::value_t::discarded;
json const empty_json_string = json::value_t::string;
json const empty_json_object = json::object();


// Recursively patches existing JSON with contents of 'overrides', which must be a JSON object.
// A 'null' value inside erases previous contents. Any other value overrides.
// See: https://json.nlohmann.me/api/basic_json/merge_patch/
// Example below, for load_app_settings.
// Use 'what' to denote what it is we're patching in, if a failure happens. The std::runtime_error will populate with
// it.
// 
// NOTE: called 'override' to agree with terminology elsewhere, and to avoid collisions with the json patch(),
// merge_patch(), existing functions.
//
void json_base::override( json_ref patches, std::string const & what )
{
    if( ! patches.is_object() )
    {
        std::string context = what.empty() ? std::string( "patch", 5 ) : what;
        throw std::runtime_error( context + ": expecting an object; got " + patches.dump() );
    }

    try
    {
        static_cast< json * >( this )->merge_patch( patches );
    }
    catch( std::exception const & e )
    {
        std::string context = what.empty() ? std::string( "patch", 5 ) : what;
        throw std::runtime_error( "failed to merge " + context + ": " + e.what() );
    }
}


// Load the contents of a file into a JSON object.
// 
// Throws if any errors are encountered with the file or its contents.
// Returns the contents. If the file wasn't there, returns missing_json.
//
json json_config::load_from_file( std::string const & filename )
{
    std::ifstream f( filename );
    if( f.good() )
    {
        try
        {
            return json::parse( f );
        }
        catch( std::exception const & e )
        {
            throw std::runtime_error( "failed to load configuration file (" + filename
                                      + "): " + std::string( e.what() ) );
        }
    }
    return missing_json;
}



// Loads configuration settings from 'global' content (loaded by load_from_file()?).
// E.g., a configuration file may contain:
//     {
//         "context": {
//             "dds": {
//                 "enabled": false,
//                 "domain" : 5
//             }
//         },
//         ...
//     }
// This function will load a specific key 'context' inside and return it. The result will be a disabling of dds:
// Besides this "global" key, application-specific settings can override the global settings, e.g.:
//     {
//         "context": {
//             "dds": {
//                 "enabled": false,
//                 "domain" : 5
//             }
//         },
//         "realsense-viewer": {
//             "context": {
//                 "dds": { "enabled": null }
//             }
//         },
//         ...
//     }
// If the current application is 'realsense-viewer', then the global 'context' settings will be patched with the
// application-specific 'context' and returned:
//     {
//         "dds": {
//             "domain" : 5
//         }
//     }
// See rules for patching in patch().
// The 'application' is usually any single-word executable name (without extension).
// The 'subkey' is mandatory.
// The 'error_context' is used for error reporting, to show what failed. Like application, it should be a single word
// that can be used to denote hierarchy within the global json.
//
json json_config::load_app_settings( json const & global,
                                     std::string const & application,
                                     json_key const & subkey,
                                     std::string const & error_context )
{
    // Take the global subkey settings out of the configuration
    json settings;
    if( auto global_subkey = global.nested( subkey ) )
        settings.override( global_subkey, "global " + error_context + '/' + subkey );

    // Patch any application-specific subkey settings
    if( auto application_subkey = global.nested( application, subkey ) )
        settings.override( application_subkey, error_context + '/' + application + '/' + subkey );

    return settings;
}


// Same as above, but automatically takes the application name from the executable-name
//
json json_config::load_settings( json const & global,
                                 json_key const & subkey,
                                 std::string const & error_context )
{
    return load_app_settings( global, rsutils::os::executable_name(), subkey, error_context );
}


}  // namespace rsutils
