// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once

#include <string>
#include <tuple>
#include <mutex>
#include <fstream>
#include <list>

#include "rosbag_content.h"
#include "print_helpers.h"

namespace rosbag_inspector
{
    class files_container
    {
    public:
        size_t size()
        {
            std::lock_guard<std::mutex> lock(mutex);
            return m_files.size();
        }
        rosbag_content& operator[](int index)
        {
            std::lock_guard<std::mutex> lock(mutex);
            if (index >= m_files.size())
                throw std::out_of_range(std::string("index: ") + std::to_string(index));

            auto itr = m_files.begin();
            std::advance(itr, index);
            return *itr;
        }
        int remove_file(int index)
        {
            std::lock_guard<std::mutex> lock(mutex);
            if (index >= m_files.size())
                throw std::out_of_range(std::string("index: ") + std::to_string(index));

            auto it_to_remove = m_files.begin();
            std::advance(it_to_remove, index);
            auto it = m_files.erase(it_to_remove);
            return std::distance(m_files.begin(), it);
        }
        void AddFiles(std::vector<std::string> const& files)
        {
            std::lock_guard<std::mutex> lock(mutex);

            for (auto&& file : files)
            {
                try
                {
                    if (std::find_if(m_files.begin(), m_files.end(), [file](const rosbag_content& r) { return r.path == file; }) != m_files.end())
                    {
                        throw std::runtime_error(tmpstringstream() << "File \"" << file << "\" already loaded");
                    }
                    m_files.emplace_back(file);
                }
                catch (const std::exception& e)
                {
                    last_error_message += e.what();
                    last_error_message += "\n";
                }
            }
        }
        bool has_errors() const
        {
            return !last_error_message.empty();
        }
        std::string get_last_error()
        {
            return last_error_message;
        }
        void clear_errors()
        {
            last_error_message.clear();
        }
    private:
        std::mutex mutex;
        std::list<rosbag_content> m_files;
        std::string last_error_message;
    };
}