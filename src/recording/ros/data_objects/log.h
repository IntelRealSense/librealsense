// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once

#include <string>
#include <recording/ros/file_types.h>
#include <rosgraph_msgs/Log.h>
#include "stream_data.h"

//TODO: Ziv, remove this file

namespace rs
{
    namespace file_format
    {
        namespace ros_data_objects
        {
            struct log_info
            {
                uint8_t level;
                std::string message;
                std::string file;
                std::string function;
                uint32_t line;
                file_types::nanoseconds capture_time;

            };

            class log : public sample
            {
            public:
                virtual ~log() = default;

                static std::string get_topic()
                {
                    return "/log";
                }

                log(const log_info& info)  : m_info(info) {}


                virtual file_types::sample_type get_type() const override
                {
                    return file_types::st_log;
                }

                virtual void write_data(ros_writer& file) override
                {
                    rosgraph_msgs::Log msg;
                    msg.level = m_info.level;
                    msg.msg = m_info.message;
                    msg.file = m_info.file;
                    msg.function = m_info.function;
                    msg.line = m_info.line;

                    file.write(get_topic(), m_info.capture_time, msg);
                }


                virtual log_info get_info() const{
                    return m_info;
                }

                virtual void set_info(const log_info& info){
                    m_info = info;
                }
            private:
                log_info m_info;
            };
        }
    }
}

