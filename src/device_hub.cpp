// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "device_hub.h"
#include <librealsense/rs2.hpp>

namespace librealsense
{
    #define devices_changed_callbacl rs2::devices_changed_callback<std::function<void(rs2::event_information& info)>>

    device_hub::device_hub(context& ctx)
        : _ctx(ctx)
    {
        _device_list = _ctx.query_devices();


        auto cb = new devices_changed_callbacl([&](rs2::event_information& info)
                   {
                       std::unique_lock<std::mutex> lock(_mutex);
                       _device_list = _ctx.query_devices();

                       // Current device will point to the first available device
                       _camera_index = 0;
                       if (_device_list.size() > 0)
                       {
                           _cv.notify_all();
                       }
                   });

        _ctx.set_devices_changed_callback({cb,  [](rs2_devices_changed_callback* p) { p->release(); }});
    }

    /**
     * If any device is connected return it, otherwise wait until next RealSense device connects.
     * Calling this method multiple times will cycle through connected devices
     */
    std::shared_ptr<device_interface> device_hub::wait_for_device()
    {
        {
            std::unique_lock<std::mutex> lock(_mutex);
            // check if there is at least one device connected
            if (_device_list.size() > 0)
            {
                // user can switch the devices by calling to wait_for_device until he get the desire device
                // _camera_index is the curr device that user want to work with

                auto dev = _device_list[_camera_index % _device_list.size()];
                _camera_index = ++_camera_index % _device_list.size();
                return dev->create_device();
            }
            else
            {
                std::cout<<"No device connected\n";
            }
        }
        // if there are no devices connected or something wrong happened while enumeration
        // wait for event of device connection
        // and do it until camera connected and succeed in its creation
        while (true)
        {
            std::unique_lock<std::mutex> lock(_mutex);
            if (_device_list.size() == 0)
            {
                _cv.wait_for(lock, std::chrono::hours(999999), [&]() {return _device_list.size()>0; });
            }
            try
            {
                return  _device_list[0]->create_device();;
            }
            catch (...)
            {
                std::cout<<"Couldn't create the device\n";
            }
        }
    }

    /**
    * Checks if device is still connected
    */
    bool device_hub::is_connected(const device_interface& dev)
    { 
        return true;
//        rs2_error* e = nullptr;

//        if (!dev)
//            return false;

//        try
//        {
//            std::unique_lock<std::mutex> lock(_mutex);

//            for (auto d : _device_list)
//            {
//                if (d.->get_device_data() == d)
//                {
//                    return 1;
//                }
//            }
//            return 0;
//        }
//        catch (...)  { return false; }
    };
}

