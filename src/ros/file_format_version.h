// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace rs
{
    namespace file_format
    {
        //TODO: refactor and make this usable
         static const std::string get_file_version_topic()
         {
             return "/FILE_VERSION";
         }

         static constexpr uint32_t get_file_version()
         {
             return 1;
         }
    }
}
