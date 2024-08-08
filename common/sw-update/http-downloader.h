// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include <cstdint>
#include <string>
#include <functional>
#include <vector>

namespace rs2
{
    namespace http
    {

        enum class callback_result { CONTINUE_DOWNLOAD, STOP_DOWNLOAD };
        typedef std::function<callback_result(uint64_t dl_current_bytes, uint64_t dl_total_bytes)> user_callback_func_type;

        struct progress_data; // Forward Declaration

                              // This class is a service class for downloading a file from an HTTP URL.
                              // The class use libcurl as a client-side URL transfer library.
        class http_downloader
        {
        public:
            http_downloader();
            ~http_downloader();

            //  The optional callback function provides 2 major capabilities:
            //    - Current status about the download progress
            //    - Control the download process (stop/continue) using the return value of the callback function (true = stop download)
            bool download_to_stream(const std::string& url, std::stringstream &output, user_callback_func_type user_callback_func = user_callback_func_type());
            bool download_to_bytes_vector(const std::string& url, std::vector<uint8_t> &output, user_callback_func_type user_callback_func = user_callback_func_type());
            bool download_to_file(const std::string& url, const std::string &file_name, user_callback_func_type user_callback_func = user_callback_func_type());

        private:
            void register_progress_call_back(progress_data &progress_record, user_callback_func_type user_callback_func);
            void set_common_options(const std::string &url);

            void* _curl;
        };
    }
}
