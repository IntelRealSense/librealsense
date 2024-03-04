// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.
#pragma once

#include <rsutils/json-fwd.h>


namespace rsutils {
namespace json_config {


// Load the contents of a file into a JSON object.
json load_from_file( std::string const & filename );

// Loads configuration settings from 'global' content
json load_app_settings( json const & global,
                        std::string const & application,
                        json_key const & subkey,
                        std::string const & error_context );

// Same as above, but automatically takes the application name from the executable-name
json load_settings( json const & global, json_key const & subkey, std::string const & error_context );


}  // namespace json_config
}  // namespace rsutils
