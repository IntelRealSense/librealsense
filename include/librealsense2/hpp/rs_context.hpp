// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#ifndef LIBREALSENSE_RS2_CONTEXT_HPP
#define LIBREALSENSE_RS2_CONTEXT_HPP

#include "rs_types.hpp"
#include "rs_record_playback.hpp"
#include "rs_processing.hpp"


namespace rs2
{
    /**
    * default librealsense context class
    * includes realsense API version as provided by RS2_API_VERSION macro
    */
    class context
    {
    public:
        context()
        {
            rs2_error* e = nullptr;
            _context = std::shared_ptr<rs2_context>(
                rs2_create_context(RS2_API_VERSION, &e),
                rs2_delete_context);
            error::handle(e);
        }

        /**
        * create a static snapshot of all connected devices at the time of the call
        * \return            the list of devices connected devices at the time of the call
        */
        device_list query_devices() const
        {
            rs2_error* e = nullptr;
            std::shared_ptr<rs2_device_list> list(
                rs2_query_devices(_context.get(), &e),
                rs2_delete_device_list);
            error::handle(e);

            return device_list(list);
        }

        /**
         * @brief Generate a flat list of all available sensors from all RealSense devices
         * @return List of sensors
         */
        std::vector<sensor> query_all_sensors() const
        {
            std::vector<sensor> results;
            for (auto&& dev : query_devices())
            {
                auto sensors = dev.query_sensors();
                std::copy(sensors.begin(), sensors.end(), std::back_inserter(results));
            }
            return results;
        }

        /**
        * \return            the time at specific time point, in live and redord contextes it will return the system time and in playback contextes it will return the recorded time
        */
        double get_time()
        {
            rs2_error* e = nullptr;
            auto time = rs2_get_context_time(_context.get(), &e);

            error::handle(e);

            return time;
        }

        device get_sensor_parent(const sensor& s) const
        {
            rs2_error* e = nullptr;
            std::shared_ptr<rs2_device> dev(
                rs2_create_device_from_sensor(s._sensor.get(), &e),
                rs2_delete_device);
            error::handle(e);
            return device{ dev };
        }

        rs2_extrinsics get_extrinsics(const sensor& from, const sensor& to) const
        {
            return get_extrinsics(from.get_stream_profiles().front(),
                                  to.get_stream_profiles().front());
        }

        rs2_extrinsics get_extrinsics(const stream_profile& from, const stream_profile& to) const
        {
            rs2_error* e = nullptr;
            rs2_extrinsics res;
            rs2_get_extrinsics(from.get(), to.get(), &res, &e);
            error::handle(e);
            return res;
        }

        /**
        * register devices changed callback
        * \param[in] callback   devices changed callback
        */
        template<class T>
        void set_devices_changed_callback(T callback) const
        {
            rs2_error* e = nullptr;
            rs2_set_devices_changed_callback_cpp(_context.get(),
                new devices_changed_callback<T>(std::move(callback)), &e);
            error::handle(e);
        }

        template<class T>
        processing_block create_processing_block(T processing_function) const
        {
            rs2_error* e = nullptr;
            std::shared_ptr<rs2_processing_block> block(
                rs2_create_processing_block(_context.get(),
                    new frame_processor_callback<T>(processing_function),
                    &e),
                rs2_delete_processing_block);
            error::handle(e);

            return processing_block(block);
        }

        pointcloud create_pointcloud() const
        {
            rs2_error* e = nullptr;
            std::shared_ptr<rs2_processing_block> block(
                    rs2_create_pointcloud(_context.get(), &e),
                    rs2_delete_processing_block);
            error::handle(e);

            return pointcloud(processing_block{ block });
        }

        /**
         * Creates a device from a RealSense file
         *
         * On successful load, the device will be appended to the context and a devices_changed event triggered
         * @param file  Path to a RealSense File
         * @return A playback device matching the given file
         */
        playback load_device(const std::string& file)
        {
            rs2_error* e = nullptr;
            auto device = std::shared_ptr<rs2_device>(
                rs2_context_add_device(_context.get(), file.c_str(), &e),
                rs2_delete_device);
            rs2::error::handle(e);

            return playback { device };
        }

        void unload_device(const std::string& file)
        {
            rs2_error* e = nullptr;
            rs2_context_remove_device(_context.get(), file.c_str(), &e);
            rs2::error::handle(e);
        }
    protected:
        friend class pipeline;
        std::shared_ptr<rs2_context> _context;
    };
}
#endif // LIBREALSENSE_RS2_CONTEXT_HPP
