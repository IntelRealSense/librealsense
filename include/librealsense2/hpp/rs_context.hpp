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


        device get_sensor_parent(const sensor& s) const
        {
            rs2_error* e = nullptr;
            std::shared_ptr<rs2_device> dev(
                rs2_create_device_from_sensor(s._sensor.get(), &e),
                rs2_delete_device);
            error::handle(e);
            return device{ dev };
        }

        /**
        * register devices changed callback
        * \param[in] callback   devices changed callback
        */
        template<class T>
        void set_devices_changed_callback(T callback)
        {
            rs2_error* e = nullptr;
            rs2_set_devices_changed_callback_cpp(_context.get(),
                new devices_changed_callback<T>(std::move(callback)), &e);
            error::handle(e);
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
        friend class device_hub;

        context(std::shared_ptr<rs2_context> ctx)
            : _context(ctx)
        {}
        std::shared_ptr<rs2_context> _context;
    };

    /**
    * device_hub class - encapsulate the handling of connect and disconnect events
    */
    class device_hub
    {
    public:
        explicit device_hub(context ctx)
            : _ctx(std::move(ctx))
        {
            rs2_error* e = nullptr;
            _device_hub = std::shared_ptr<rs2_device_hub>(
                rs2_create_device_hub(_ctx._context.get(), &e),
                rs2_delete_device_hub);
            error::handle(e);
        }

        /**
        * If any device is connected return it, otherwise wait until next RealSense device connects.
        * Calling this method multiple times will cycle through connected devices
        */
        device wait_for_device() const
        {
            rs2_error* e = nullptr;
            std::shared_ptr<rs2_device> dev(
                rs2_device_hub_wait_for_device(_ctx._context.get(), _device_hub.get(), &e),
                rs2_delete_device);

            error::handle(e);

            return device(dev);
          
        }

        /**
        * Checks if device is still connected
        */
        bool is_connected(const device& dev) const
        {
            rs2_error* e = nullptr;
            auto res = rs2_device_hub_is_device_connected(_device_hub.get(), dev._dev.get(), &e);
            error::handle(e);

            return res > 0 ? true : false;
            
        }
    private:
        context _ctx;
        std::shared_ptr<rs2_device_hub> _device_hub;
    };

}
#endif // LIBREALSENSE_RS2_CONTEXT_HPP
