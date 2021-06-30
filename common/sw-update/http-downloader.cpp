// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.



#ifdef CHECK_FOR_UPDATES
#include <sstream>
#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
#include <curl/curl.h>
#include <curl/easy.h>
#include "types.h"
#endif // CHECK_FOR_UPDATES

#include "http-downloader.h"


namespace rs2
{
    namespace http
    {

#ifndef CHECK_FOR_UPDATES
        // Dummy functions
        http_downloader::http_downloader() {}
        http_downloader::~http_downloader() {}
        bool http_downloader::download_to_stream(const std::string& url, std::stringstream &output, user_callback_func_type user_callback_func) { return false; }
        bool http_downloader::download_to_file(const std::string& url, const std::string &file_name, user_callback_func_type user_callback_func) { return false; }
        bool http_downloader::download_to_bytes_vector(const std::string& url, std::vector<uint8_t> &output, user_callback_func_type user_callback_func) { return false; }

#else

        std::mutex initialize_mutex;
        static const curl_off_t HALF_SEC = 500000; // User call back function delay
        static const int CONNECT_TIMEOUT = 5L; // Libcurl connection timeout 5 [Sec]

        struct progress_data {
            curl_off_t last_run_time;
            user_callback_func_type user_callback_func;
            CURL *curl;
        };


        size_t stream_write_callback(void *input_stream, size_t size, size_t nmemb, void *output_stream)
        {
            if (input_stream && output_stream)
            {
                std::string data((const char*)input_stream, (size_t)size * nmemb);
                *((std::stringstream*)output_stream) << data;
                return size * nmemb;
            }
            return 0; // Error
        }

        size_t vector_write_callback(void *input_stream, size_t size, size_t nmemb, void *output_vec)
        {
            uint8_t* source_bytes(static_cast<uint8_t*>(input_stream));

            if (input_stream && output_vec)
            {
                int total_size((int)(size * nmemb));
                while (total_size > 0)
                {
                    static_cast<std::vector<uint8_t> *>(output_vec)->push_back(*source_bytes);
                    source_bytes++;
                    --total_size;
                }

                return size * nmemb;
            }
            return 0; // Error
        }

        size_t file_write_callback(void *input_stream, size_t size, size_t nmemb, void *output)
        {

            if (input_stream && output)
            {
                std::ofstream &out_stream(*static_cast<std::ofstream*> (output));

                size_t num_of_bytem(nmemb*size);
                out_stream.write((char *)input_stream, num_of_bytem);
                return size * nmemb;

            }
            return 0; // Error
        }

        // This function will be called if CURLOPT_NOPROGRESS is set to 0
        // Return value: 0 = continue download / 1 = stop download
        int progress_callback(void *p, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow)
        {
            progress_data *myp = static_cast<progress_data *>(p);
            CURL *curl(myp->curl);
            curl_off_t curtime(0);
            curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME_T, &curtime);
            if (dltotal != 0 && (curtime - myp->last_run_time > HALF_SEC))
            {
                myp->last_run_time = curtime;
                return myp->user_callback_func(static_cast<uint64_t>(dlnow),
                    static_cast<uint64_t>(dltotal)) == callback_result::CONTINUE_DOWNLOAD ? 0 : 1;
            }
            else
            {
                return 0;
            }
        }

        http_downloader::http_downloader() : _curl(nullptr)
        {
            // Protect curl_easy_init() it is not considers thread safe
            std::lock_guard<std::mutex> lock(initialize_mutex);
            _curl = curl_easy_init();
        }

        http_downloader::~http_downloader()
        {
            std::lock_guard<std::mutex> lock(initialize_mutex);
            curl_easy_cleanup(_curl);
        }

        bool http_downloader::download_to_stream(const std::string& url, std::stringstream &output, user_callback_func_type user_callback_func)
        {
            if (!_curl) return false;

            set_common_options(url);
            curl_easy_setopt(_curl, CURLOPT_WRITEFUNCTION, stream_write_callback);
            curl_easy_setopt(_curl, CURLOPT_WRITEDATA, &output);
            curl_easy_setopt(_curl, CURLOPT_SSL_VERIFYPEER ,0L);
            curl_easy_setopt(_curl, CURLOPT_SSL_VERIFYHOST ,0L);

            progress_data progress_record; // Should stay here - "curl_easy_perform" use it
            if (user_callback_func)
            {
                register_progress_call_back(progress_record, user_callback_func);
            }
            auto res = curl_easy_perform(_curl);

            if (CURLE_OK != res)
            {
                LOG_ERROR("Download error from URL: " + url + ", error info: " + std::string(curl_easy_strerror(res)));
                return false;
            }
            return true;
        }

        bool http_downloader::download_to_bytes_vector(const std::string& url, std::vector<uint8_t> &output, user_callback_func_type user_callback_func)
        {
            if (!_curl) return false;

            set_common_options(url);
            curl_easy_setopt(_curl, CURLOPT_WRITEFUNCTION, vector_write_callback);
            curl_easy_setopt(_curl, CURLOPT_WRITEDATA, &output);

            progress_data progress_record; // Should stay here - "curl_easy_perform" use it
            if (user_callback_func)
            {
                register_progress_call_back(progress_record, user_callback_func);
            }
            auto res = curl_easy_perform(_curl);

            if (CURLE_OK != res)
            {
                LOG_ERROR("Download error from URL: " + url + ", error info: " + std::string(curl_easy_strerror(res)));
                return false;
            }
            return true;
        }


        bool http_downloader::download_to_file(const std::string& url, const std::string &file_name, user_callback_func_type user_callback_func)
        {
            if (!_curl) return false;

            /* open the file */
            std::ofstream out_file(file_name, std::ios::out);

            if (out_file.good())
            {
                set_common_options(url);
                curl_easy_setopt(_curl, CURLOPT_WRITEFUNCTION, file_write_callback);
                curl_easy_setopt(_curl, CURLOPT_WRITEDATA, &out_file);

                progress_data progress_record; // Should stay here - "curl_easy_perform" use it
                if (user_callback_func)
                {
                    register_progress_call_back(progress_record, user_callback_func);
                }
                auto res = curl_easy_perform(_curl);
                out_file.close();

                if (CURLE_OK != res)
                {
                    LOG_ERROR("Download error from URL: " + url + ", error info: " + std::string(curl_easy_strerror(res)));
                    return false;
                }
            }
            else
            {
                LOG_ERROR("Download error - Cannot open local file: " + file_name);
                return false;
            }

            return true;
        }

        void http_downloader::set_common_options(const std::string &url)
        {
            curl_easy_setopt(_curl, CURLOPT_URL, url.c_str());                  // provide the URL to use in the request
            curl_easy_setopt(_curl, CURLOPT_FOLLOWLOCATION, 1L);                // follow HTTP 3xx redirects
            curl_easy_setopt(_curl, CURLOPT_NOSIGNAL, 1);                       // skip all signal handling
            curl_easy_setopt(_curl, CURLOPT_CONNECTTIMEOUT, CONNECT_TIMEOUT);   // timeout for the connect phase
            curl_easy_setopt(_curl, CURLOPT_FAILONERROR, 1L);                   // request failure on HTTP response >= 400
            curl_easy_setopt(_curl, CURLOPT_NOPROGRESS, 1L);                    // switch off the progress meter
        }
        void http_downloader::register_progress_call_back(progress_data &progress_record, user_callback_func_type user_callback_func)
        {
            progress_record = { 0, user_callback_func, _curl };
            curl_easy_setopt(_curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
            curl_easy_setopt(_curl, CURLOPT_XFERINFODATA, &progress_record);
            curl_easy_setopt(_curl, CURLOPT_NOPROGRESS, 0L);
        }
#endif
    }
}
