// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.
#pragma once

#include <string>


namespace rsutils {
namespace os {


enum class special_folder
{
    user_desktop,
    user_documents,
    user_pictures,
    user_videos,
    temp_folder,
    app_data
};


// Get the path to one of the special folders listed above. These differ based on operating system.
// Meant to be used to append a filename to it: may or may not end with a directory separator slash!
// Throws invalid_argument if an invalid folder was passed in.
// Throws runtime_error on failure.
//
std::string get_special_folder( special_folder );


}  // namespace os
}  // namespace rsutils
