// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include <string>


namespace rsutils {
namespace os {


// Returns the path to the executable currently running
std::string executable_path();

// Returns the filename component of the given path, with or without the extension
std::string base_name( std::string path, bool with_extension = false );

// Returns the filename component of the executable currently running
inline std::string executable_name( bool with_extension = false )
{
    return base_name( executable_path(), with_extension );
}


}
}
