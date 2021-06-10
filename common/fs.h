// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#pragma once

#include <string>

namespace rs2 {

bool directory_exists( const char * dir );
std::string get_file_name( const std::string & path );

enum special_folder
{
    user_desktop,
    user_documents,
    user_pictures,
    user_videos,
    temp_folder,
    app_data
};

std::string get_timestamped_file_name();
std::string get_folder_path( special_folder f );

}  // namespace rs2
