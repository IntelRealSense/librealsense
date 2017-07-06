// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once

#include <core/serialization.h>
#include <ros/realsense_file/include/file_types.h>
using namespace rs::file_format;
namespace librealsense
{
    class ros_device_serializer: public device_serializer
    {
    public:
        ros_device_serializer(std::string file);

        class ros_writer: public device_serializer::writer
        {
        public:
            void write_device_description(const device_snapshot& device_description) override;
            void write(storage_data data) override;
            void reset() override;
        private:
            void write_extension_snapshot(uint32_t id, std::shared_ptr<extension_snapshot> shared_ptr);
        };

        class ros_reader: public device_serializer::reader
        {
        public:
            device_snapshot query_device_description() override;
            storage_data read() override;
            void seek_to_time(std::chrono::nanoseconds time) override;
            std::chrono::nanoseconds query_duration() const override;
            void reset() override;
        };

        std::shared_ptr <writer> get_writer() override;
        std::shared_ptr <reader> get_reader() override;

    private:
        std::string m_file;
        std::shared_ptr <writer> m_writer; //TODO: Ziv, consider lazy<T>
        std::shared_ptr <reader> m_reader;
    };
}