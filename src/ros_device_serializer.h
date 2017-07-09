// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once

#include <core/serialization.h>
#include <ros/stream_playback.h>
#include "ros/stream_recorder.h"

namespace librealsense
{
    class ros_device_serializer: public device_serializer
    {
    public:
        ros_device_serializer(const std::string& file);

        //////////////////////////////////////////////////////////
        ///      ros_device_serializer::ros_writer             ///
        //////////////////////////////////////////////////////////

        class ros_writer: public device_serializer::writer
        {
        public:
            ros_writer(const std::string& file);
            void write_device_description(const device_snapshot& device_description) override;
            void write(storage_data data) override;
            void reset() override;
        private:
            void write_extension_snapshot(uint32_t id, std::shared_ptr<extension_snapshot> shared_ptr);

            rs::file_format::file_types::microseconds m_first_frame_time;
            rs::file_format::stream_recorder m_stream_recorder;
        };


        //////////////////////////////////////////////////////////
        ///      ros_device_serializer::ros_reader             ///
        //////////////////////////////////////////////////////////

        class ros_reader: public device_serializer::reader
        {
        public:
            ros_reader(const std::string& file);
            device_snapshot query_device_description() override;
            storage_data read() override;
            void seek_to_time(std::chrono::nanoseconds time) override;
            std::chrono::nanoseconds query_duration() const override;
            void reset() override;
        private:
            rs::file_format::stream_playback m_stream_playback;
        };

        std::shared_ptr <writer> get_writer() override;
        std::shared_ptr <reader> get_reader() override;

    private:
        std::string m_file;
        std::shared_ptr <writer> m_writer; //TODO: Ziv, consider lazy<T>
        std::shared_ptr <reader> m_reader;
    };
}