// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include <librealsense2/rs.hpp>

#include "device_hub.h"


namespace librealsense
{

    typedef rs2::devices_changed_callback<std::function<void(rs2::event_information& info)>> hub_devices_changed_callback;

    std::vector<std::shared_ptr<device_info>> filter_by_vid(std::vector<std::shared_ptr<device_info>> devices , int vid)
    {
        std::vector<std::shared_ptr<device_info>> result;
        for (auto dev : devices)
        {
            auto data = dev->get_device_data();
            for (auto uvc : data.uvc_devices)
            {
                if (uvc.vid == vid || vid == 0)
                {
                    result.push_back(dev);
                    break;
                }
            }
        }
        return result;
    }

    device_hub::device_hub(std::shared_ptr<librealsense::context> ctx, int vid)
        : _ctx(ctx), _vid(vid)
    {
        _device_list = filter_by_vid(_ctx->query_devices(), _vid);

        auto cb = new hub_devices_changed_callback([&](rs2::event_information& info)
                   {
                        std::unique_lock<std::mutex> lock(_mutex);

                        _device_list = filter_by_vid(_ctx->query_devices(), _vid);

                        // Current device will point to the first available device
                        _camera_index = 0;
                        if (_device_list.size() > 0)
                        {
                           _cv.notify_all();
                        }
                    });

        _ctx->set_devices_changed_callback({cb,  [](rs2_devices_changed_callback* p) { p->release(); }});
    }

    std::shared_ptr<device_interface> device_hub::create_device(std::string serial)
    {
        for(auto i = 0; i< _device_list.size(); i++)
        {

            // user can switch the devices by calling to wait_for_device until he get the desire device
            // _camera_index is the curr device that user want to work with

            auto d = _device_list[ (_camera_index + i) % _device_list.size()];
            auto dev = d->create_device();

            if(serial.size() > 0 )
            {
                auto new_serial = dev->get_info(RS2_CAMERA_INFO_SERIAL_NUMBER);

                if(serial == new_serial)
                {
                    _camera_index = ++_camera_index % _device_list.size();
                    return dev;
                }
            }
            else
            {
                _camera_index = ++_camera_index % _device_list.size();
                 return dev;
            }
        }
        return nullptr;
    }


    /**
     * If any device is connected return it, otherwise wait until next RealSense device connects.
     * Calling this method multiple times will cycle through connected devices
     */
    std::shared_ptr<device_interface> device_hub::wait_for_device(unsigned int timeout_ms, std::string serial)
    {
        {
            std::unique_lock<std::mutex> lock(_mutex);
            // check if there is at least one device connected
            if(_device_list.size()>0)
            {
                auto res = create_device(serial);
                if(res)
                    return res;
            }
        }
        // if there are no devices connected or something wrong happened while enumeration
        // wait for event of device connection
        // and do it until camera connected and succeed in its creation

        std::shared_ptr<device_interface> res;

        std::unique_lock<std::mutex> lock(_mutex);

        if(!_cv.wait_for(lock, std::chrono::milliseconds(timeout_ms), [&]()
        {
            res = nullptr;
            if(_device_list.size()>0)
            {
                res = create_device(serial);
            }
            return res != nullptr;

        }))
        {
            throw std::runtime_error("No device connected");
        }

        return res;

    }

    /**
    * Checks if device is still connected
    */
    bool device_hub::is_connected(const device_interface& dev)
    {
        try
        {
            std::unique_lock<std::mutex> lock(_mutex);

            for (auto d : _device_list)
            {
                if (d->get_device_data() == dev.get_device_data())
                {
                    return true;
                }
            }
            return false;
        }
        catch (...)  { return false; }
    }
}

