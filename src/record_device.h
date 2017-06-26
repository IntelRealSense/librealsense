// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once
#include "core.h"
#include "archive.h"
#include "concurrency.h"
namespace rsimpl2
{
    class extension_metadata
    {
        //TODO: should be serialize-able, update-able, extendable to the actual extension
    };
    class query_metadata_interface
    {
    public:
        virtual ~query_metadata_interface() = default;

        /**
         * @brief Get the number of available metadata objects
         * @return Number of available metadata objects
         */
        virtual uint32_t metadata_count() const = 0;

        /**
         * @brief Iterate the available metadata objects
         * @param[in] index  Index of the requested metadata object
         * @param[out] obj  On successful execution will contain the metadata object of the requested index
         * @return True on successful execution
         */
        virtual std::vector<std::shared_ptr<extension_metadata>> query_metadata() const = 0;
    };

    /**
     * sensor_description_interface is an abstraction for metadata information of sensors
     * A sensor description object will contain all the information of a sensor's extensions
     *
     * This is mainly in use for serializaion of sensors
     */
    class sensor_description_interface : public query_metadata_interface

    {
    public:
        virtual ~sensor_description_interface() = default;
    };


    /**
     * device_description_interface is an abstraction for metadata information of a device
     * A device description object will contain all the information of a device's extensions and its sensors descriptions
     *
     * This is mainly in use for serializaion of devices
     */
    class device_description_interface : public query_metadata_interface
    {
    public:
        virtual ~device_description_interface() = default;

        /**
         * @brief Get the number of available sensors descriptions
         * @return Number of available sensors descriptions
         */
        virtual uint32_t sensor_description_count() const = 0;

        /**
         * @brief  Iterate the available sensors descriptions
         * @param index  Index of the requested sensors descriptions
         * @param description On successful execution will contain the sensors descriptions of the requested index
         * @return True on successful execution
         */
        virtual bool query_sensor_description(uint32_t index, sensor_description_interface** description) const = 0;
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

        virtual ~device_serializer() = default;

        virtual void reset() = 0;

        virtual std::shared_ptr<device_description_interface> query_device_description() = 0;
        virtual void write_device_description(std::shared_ptr<device_description_interface> device_description) = 0;

        virtual storage_data read() = 0;
        virtual void write(storage_data data) = 0;

        virtual void seek_to_time(std::chrono::nanoseconds time) = 0;
        virtual std::chrono::nanoseconds query_duration() const = 0;
        
    };
    class record_sensor : public sensor_interface
    {
    public:
        record_sensor(std::shared_ptr<sensor_interface> sensor, frame_callback_ptr record_data_func);
        virtual ~record_sensor();
    };

    class record_device : public device_interface
    {
    public:
        record_device(std::shared_ptr<device_interface> device, std::shared_ptr<device_serializer> serializer);
        virtual ~record_device();

        sensor_interface& get_sensor(unsigned int i) override;
        unsigned int get_sensors_count() const override;

    private:
        std::shared_ptr<device_interface> m_device;
        std::vector<std::shared_ptr<record_sensor>> m_sensors;

        lazy<std::shared_ptr<dispatcher>> m_write_thread;
        std::shared_ptr<device_serializer> m_writer;

        std::chrono::high_resolution_clock::time_point m_capture_time_base;
        std::chrono::high_resolution_clock::duration m_record_pause_time;
        std::chrono::high_resolution_clock::time_point m_time_of_pause;

        std::mutex m_mutex;
        bool m_is_recording;


        uint64_t m_cached_data_size;
        bool m_is_first_event;
    };
}


