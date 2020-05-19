// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include <sstream>
#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>

#ifdef ENABLE_RS_AUTO_UPDATER
#include <curl/curl.h>
#include <curl/easy.h>
#endif // ENABLE_RS_AUTO_UPDATER


#include "http-downloader.h"

namespace rs2
{
#ifdef ENABLE_RS_AUTO_UPDATER
    std::mutex initialize_mutex;
    static const curl_off_t HALF_SEC = 500000; // User call back function delay
    static const int CONNECT_TIMEOUT = 5L; // Libcurl download timeout 5 [Sec]

    struct progress_data {
        curl_off_t last_run_time;
        user_callback_func_type user_callback_func;
        CURL *curl;
    };


    size_t write_callback(void *ptr, size_t size, size_t nmemb, void *stream) {
        std::string data((const char*)ptr, (size_t)size * nmemb);
        *((std::stringstream*)stream) << data;
        return size * nmemb;
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
            return myp->user_callback_func(static_cast<uint64_t>(dlnow), static_cast<uint64_t>(dltotal), static_cast<double>(curtime / 1000000.)) ? 1 : 0;
        }
        else
        {
            return 0;
        }
    }

#endif

    http_downloader::http_downloader() {
#ifdef ENABLE_RS_AUTO_UPDATER
        // Protect curl_easy_init() it is not considers thread safe
        std::lock_guard<std::mutex> lock(initialize_mutex);
        _curl = curl_easy_init();
#endif
    }

    http_downloader::~http_downloader() {
#ifdef ENABLE_RS_AUTO_UPDATER

        std::lock_guard<std::mutex> lock(initialize_mutex);
        curl_easy_cleanup(_curl);
#endif
    }

    bool http_downloader::download_to_stream(const std::string& url, std::stringstream &output, user_callback_func_type user_callback_func)
    {
#ifdef ENABLE_RS_AUTO_UPDATER
        set_common_options(url);
        curl_easy_setopt(_curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(_curl, CURLOPT_WRITEDATA, &output);
        
        progress_data progress_record; // Should stay here - "curl_easy_perform" use it
        if (user_callback_func)
        {
            register_progress_call_back(progress_record, user_callback_func);
        }
        auto res = curl_easy_perform(_curl);

        if(CURLE_OK != res)
        {
            throw std::runtime_error("Download error - " + std::string(curl_easy_strerror(res)));
        }
        return true;
#else
        return false;
#endif
    }

    bool http_downloader::download_to_file(const std::string& url, const std::string &file_name, user_callback_func_type user_callback_func)
    {
#ifdef ENABLE_RS_AUTO_UPDATER
        /* open the file */
        FILE * out_file = fopen(file_name.c_str(), "wb");
        if (out_file)
        {
            set_common_options(url);
            curl_easy_setopt(_curl, CURLOPT_WRITEDATA, out_file);
           
            progress_data progress_record; // Should stay here - "curl_easy_perform" use it
            if (user_callback_func)
            {
                register_progress_call_back(progress_record, user_callback_func);
            }
            auto res = curl_easy_perform(_curl);

            fclose(out_file);

            if (CURLE_OK != res)
            {
                throw std::runtime_error("Download error - " + std::string(curl_easy_strerror(res)));
            }
        }
        else
        {
            return false;
        }

        return true;
#else
        return false;
#endif
    }

    void http_downloader::set_common_options(const std::string &url)
    {
#ifdef ENABLE_RS_AUTO_UPDATER
        curl_easy_setopt(_curl, CURLOPT_URL, url.c_str());                  // provide the URL to use in the request
        curl_easy_setopt(_curl, CURLOPT_FOLLOWLOCATION, 1L);                // follow HTTP 3xx redirects
        curl_easy_setopt(_curl, CURLOPT_NOSIGNAL, 1);                       // skip all signal handling
        curl_easy_setopt(_curl, CURLOPT_CONNECTTIMEOUT, CONNECT_TIMEOUT);   // timeout for the connect phase
        curl_easy_setopt(_curl, CURLOPT_FAILONERROR, 1L);                   // request failure on HTTP response >= 400
        curl_easy_setopt(_curl, CURLOPT_NOPROGRESS, 1L);                    // switch off the progress meter
#endif
    }
    void http_downloader::register_progress_call_back(progress_data &progress_record, user_callback_func_type user_callback_func)
    {
#ifdef ENABLE_RS_AUTO_UPDATER
        progress_record = { 0, user_callback_func, _curl };
        curl_easy_setopt(_curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
        curl_easy_setopt(_curl, CURLOPT_XFERINFODATA, &progress_record);
        curl_easy_setopt(_curl, CURLOPT_NOPROGRESS, 0L);
#endif
    }

}