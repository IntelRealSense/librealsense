// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once
#include <core/roi.h>
#include <core/extension.h>
#include "core/streaming.h"
#include "archive.h"
#include "concurrency.h"
#include "sensor.h"
namespace rsimpl2
{
    template <typename T,typename P>
    T* As(P* ptr)
    {
        return dynamic_cast<T*>(ptr);
    }

    /**
     * Deriving classes are expected to return an extension_snapshot
     * @tparam T
     */
    template <typename T>
    class recordable
    {
        virtual void create_snapshot(std::shared_ptr<T>& snapshot) = 0;
    };

    class extension_snapshot
    {
        virtual void update(std::shared_ptr<rsimpl2::extension_interface> ext) = 0;
    };

    template <typename T>
    class extension_snapshot_base : public extension_snapshot
    {
    public:
        void update(std::shared_ptr<rsimpl2::extension_interface> ext) override
        {
            auto api = As<T, rsimpl2::extension_interface>(ext.get());
            if(api == nullptr)
            {
                throw std::runtime_error("TODO: write me");
            }
            update_self(api);
        }
    protected:
        virtual void update_self(T* info_api) = 0;
    };



    //TODO: CR this class, created as poc
    class info_snapshot : public extension_snapshot_base<info_interface>, public info_container
    {
    public:
        info_snapshot(info_interface* info_api)
        {
            update_self(info_api);
        }
    private:
        void update_self(info_interface* info_api)
        {
            for (int i = 0; i < RS2_CAMERA_INFO_COUNT; ++i)
            {
                rs2_camera_info info = static_cast<rs2_camera_info>(i);
                if(info_api->supports_info(info))
                {
                    register_info(info, info_api->get_info(info));
                }
            }
        }
    };

    class sensor_metadata
    {
    public:
        sensor_metadata(const std::vector<std::shared_ptr<extension_snapshot>>& sensor_extensios)
            :
            m_extensios(sensor_extensios)
        {

        }

        std::vector<std::shared_ptr<rsimpl2::extension_snapshot>> get_extensions_snapshots() const
        {

        }
    private:
        std::vector<std::shared_ptr<extension_snapshot>> m_extensios;
    };

    class device_snapshot
    {
    public:
        device_snapshot(const std::vector<std::shared_ptr<extension_snapshot>>& device_extensios,
                        const std::vector<sensor_metadata>& sensors_metadata)
            :
            m_extensios(device_extensios),
            m_sensors_metadata(sensors_metadata)
        {

        }
        std::vector<sensor_metadata> query_sensor_metadata() const
        {
            return m_sensors_metadata;
        }
        std::vector<std::shared_ptr<extension_snapshot>> query_metadata() const
        {
            return m_extensios;
        }
    private:
        std::vector<std::shared_ptr<extension_snapshot>> m_extensios;
        std::vector<sensor_metadata> m_sensors_metadata;
    };

    class device_serializer
    {
    public:
        struct storage_data
        {
            std::chrono::nanoseconds timestamp;
            size_t sensor_index;
            std::shared_ptr<frame_interface> frame;
        };

        class writer
        {
        public:
            virtual void write_device_description(const device_snapshot& device_description) = 0;
            virtual void write(storage_data data) = 0;
            virtual void reset() = 0;
            virtual ~writer() = default;
        };
        class reader
        {
        public:
            virtual device_snapshot query_device_description() = 0;
            virtual storage_data read() = 0;
            virtual void seek_to_time(std::chrono::nanoseconds time) = 0;
            virtual std::chrono::nanoseconds query_duration() const = 0;
            virtual void reset() = 0;
            virtual ~reader() = default;
        };

        virtual std::shared_ptr<writer> get_writer() = 0;
        virtual std::shared_ptr<writer> get_reader() = 0;
        virtual ~device_serializer() = default;
    };

    class ros_device_serializer_impl : public device_serializer
    {
    public:
        ros_device_serializer_impl(std::string file);

        class ros_writer : public device_serializer::writer
        {
        public:
            void write_device_description(const device_snapshot& device_description) override;
            void write(storage_data data) override;
            void reset() override;
        };
        class ros_reader : public device_serializer::reader
        {
        public:
            device_snapshot query_device_description() override;
            storage_data read() override;
            void seek_to_time(std::chrono::nanoseconds time) override;
            std::chrono::nanoseconds query_duration() const override;
            void reset() override;
        };
        std::shared_ptr<writer> get_writer() override;
        std::shared_ptr<writer> get_reader() override;

    private:
        std::string m_file;
    };
    class record_sensor : public sensor_interface
    {
    public:
        using frame_interface_callback_t = std::function<void(std::shared_ptr<frame_interface>)>;

        record_sensor(sensor_interface& sensor, frame_interface_callback_t on_frame);
        virtual ~record_sensor();

        std::vector<stream_profile> get_principal_requests() override;
        void open(const std::vector<stream_profile>& requests) override;
        void close() override;
        option& get_option(rs2_option id) override;
        const option& get_option(rs2_option id) const override;
        const std::string& get_info(rs2_camera_info info) const override;
        bool supports_info(rs2_camera_info info) const override;
        bool supports_option(rs2_option id) const override;
        void register_notifications_callback(notifications_callback_ptr callback) override;
        void start(frame_callback_ptr callback) override;
        void stop() override;
        bool is_streaming() const override;

    private:
        sensor_interface& m_sensor;
        frame_interface_callback_t m_record_callback;
        bool m_is_recording;
        bool m_is_pause;
        frame_callback_ptr m_frame_callback;
    };

    class record_device : public device_interface
    {
    public:
        record_device(std::shared_ptr<device_interface> device, std::shared_ptr<device_serializer::writer> serializer);
        virtual ~record_device();

        sensor_interface& get_sensor(size_t i) override;
        size_t get_sensors_count() const override;
        const std::string& get_info(rs2_camera_info info) const override;
        bool supports_info(rs2_camera_info info) const override;
        const sensor_interface& get_sensor(size_t i) const override;
        void hardware_reset() override;
        rs2_extrinsics get_extrinsics(size_t from,
                                      rs2_stream from_stream,
                                      size_t to,
                                      rs2_stream to_stream) const override;
        static const uint64_t MAX_CACHED_DATA_SIZE = 1920 * 1080 * 4 * 30; // ~1 sec of HD video @ 30 FPS

    private:
        void write_header();
        std::chrono::nanoseconds get_capture_time();
        void write_data(size_t sensor_index, std::shared_ptr<frame_interface> f);

    private:
        std::shared_ptr<device_interface> m_device;
        std::vector<std::shared_ptr<record_sensor>> m_sensors;

        lazy<std::shared_ptr<dispatcher>> m_write_thread;
        std::shared_ptr<device_serializer::writer> m_writer;

        std::chrono::high_resolution_clock::time_point m_capture_time_base;
        std::chrono::high_resolution_clock::duration m_record_pause_time;
        std::chrono::high_resolution_clock::time_point m_time_of_pause;

        std::mutex m_mutex;
        bool m_is_recording;


        uint64_t m_cached_data_size;
        bool m_is_first_event;
        std::once_flag m_first_call_flag;
        template <typename T>
        std::vector<std::shared_ptr<extension_snapshot>> get_extensions_snapshots(T* extendable);
    };

    class mock_frame : public frame_interface
    {
        sensor_interface& m_sensor;
        frame* m_frame;
    public:
        mock_frame(sensor_interface& s, frame* f);
        double get_timestamp() const override;
        rs2_timestamp_domain get_timestamp_domain() const override;
        unsigned int get_stream_index() const override;
        const uint8_t* get_data() const override;
        size_t get_data_size() const override;
        const sensor_interface& get_sensor() const override;
    };
}


