// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include <sstream>
#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>

#include <curl/curl.h>
#include <curl/easy.h>

#include "http_downloader.h"

namespace rs2
{
    std::mutex initialize_mutex;
    static const curl_off_t HALF_SEC = 500000; // User call back func delay

    struct progress_data {
        curl_off_t last_run_time;
        std::function<bool(uint64_t, uint64_t, double)> user_callback_func;
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

    http_downloader::http_downloader() {
        // Protect curl_easy_init() it is not considerd thread safe
        std::lock_guard<std::mutex> lock(initialize_mutex);
        _curl = curl_easy_init();
    }

    http_downloader::~http_downloader() {
        std::lock_guard<std::mutex> lock(initialize_mutex);
        curl_easy_cleanup(_curl);
    }

    bool http_downloader::download_to_stream(const std::string& url, std::stringstream &output, user_callback_func_type user_callback_func)
    {
        curl_easy_setopt(_curl, CURLOPT_URL, url.c_str());    // Set Required URL
        curl_easy_setopt(_curl, CURLOPT_FOLLOWLOCATION, 1L);  // Set fllowing redirections
        curl_easy_setopt(_curl, CURLOPT_NOSIGNAL, 1);         // Disable use of signals
        curl_easy_setopt(_curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(_curl, CURLOPT_WRITEDATA, &output);
        curl_easy_setopt(_curl, CURLOPT_NOPROGRESS, 1L);
        progress_data progress_record;
        if (user_callback_func)
        {
            register_progress_call_back(progress_record, user_callback_func);
        }

        return (curl_easy_perform(_curl) == CURLE_OK);
    }

    bool http_downloader::download_to_file(const std::string& url, const std::string &file_name, user_callback_func_type user_callback_func)
    {
        CURLcode res;
        /* open the file */
        FILE * out_file = fopen(file_name.c_str(), "wb");
        if (out_file)
        {
            curl_easy_setopt(_curl, CURLOPT_URL, url.c_str());    // Set Required URL
            curl_easy_setopt(_curl, CURLOPT_WRITEDATA, out_file);
            curl_easy_setopt(_curl, CURLOPT_FOLLOWLOCATION, 1L);  // Set fllowing redirections
            curl_easy_setopt(_curl, CURLOPT_NOSIGNAL, 1);         // Disable use of signals
            curl_easy_setopt(_curl, CURLOPT_NOPROGRESS, 1L);
            progress_data progress_record;
            if (user_callback_func)
            {
                register_progress_call_back(progress_record, user_callback_func);
            }
            res = curl_easy_perform(_curl);
            fclose(out_file);
        }
        else
        {
            return false;
        }

        return (res == CURLE_OK);
    }

    void http_downloader::register_progress_call_back(progress_data &progress_record, user_callback_func_type user_callback_func)
    {
        progress_record = { 0, user_callback_func, _curl };
        curl_easy_setopt(_curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
        curl_easy_setopt(_curl, CURLOPT_XFERINFODATA, &progress_record);
        curl_easy_setopt(_curl, CURLOPT_NOPROGRESS, 0L);
    }

}