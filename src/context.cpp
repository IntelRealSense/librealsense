// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "context.h"
#include "uvc.h"
#include "r200.h"
#include "f200.h"

#include <algorithm>

rs_context::rs_context() : rs_context(0)
{
    context = rsimpl::uvc::create_context();
    enumerate_devices();
}

rsimpl::uvc::device * rs_context::get_device(const std::string & id)
{
    auto it = id_to_device.find(id);
    return it == end(id_to_device) ? nullptr : it->second.get();
}

void rs_context::flush_device(const std::string & id)
{
    auto it = id_to_device.find(id);
    if(it != end(id_to_device))
    {
        it->second = nullptr;
        LOG_INFO("Disconnected from device " << it->first);
    }
}

void rs_context::enumerate_devices()
{
    auto device_list = query_devices(context);
    for(auto & d : device_list)
    {
		if (get_vendor_id(*d) != 32902)
			continue;

        auto it = id_to_device.find(get_unique_id(*d));
        if(it == end(id_to_device))
        {
            LOG_INFO("UVC device " << get_unique_id(*d) << " detected with VID = 0x" << std::hex << get_vendor_id(*d) << " PID = 0x" << get_product_id(*d));
				
            id_to_device.insert({get_unique_id(*d), d});

            switch(get_product_id(*d))
            {
            case 2688: devices.push_back(rsimpl::make_r200_device(*this, get_unique_id(*d))); break;
            case 2662: devices.push_back(rsimpl::make_f200_device(*this, get_unique_id(*d))); break;
            case 2725: devices.push_back(rsimpl::make_sr300_device(*this, get_unique_id(*d))); break;
            }
        }
        else if(!it->second)
        {
            LOG_INFO("Reconnected to device " << it->first);
            it->second = d;

            const rsimpl::uvc::guid R200_LEFT_RIGHT_XU = {0x18682d34, 0xdd2c, 0x4073, {0xad, 0x23, 0x72, 0x14, 0x73, 0x9a, 0x07, 0x4c}};
            switch(get_product_id(*d))
            {
            case 2688: init_controls(*d, 0, R200_LEFT_RIGHT_XU); break; // TODO: Refactor so this is only done in one place
            case 2662: // TODO: F200
            case 2725: // TODO: SR300
                break;
            }
        }
        else
        {
            // Found an existing device, ignore it
        }
    }
        
    for(auto & p : id_to_device)
    {
        if(!p.second) continue;
        auto it = std::find_if(begin(device_list), end(device_list), [p](const std::shared_ptr<rsimpl::uvc::device> & d) { return p.first == get_unique_id(*d); }); 
        if(it == end(device_list))
        {
            LOG_WARNING("Lost connection to device " << p.first);
            p.second = nullptr;
        }
    }
}

// Enforce singleton semantics on rs_context

bool rs_context::singleton_alive = false;

rs_context::rs_context(int)
{
    if(singleton_alive) throw std::runtime_error("rs_context has singleton semantics, only one may exist at a time");
    singleton_alive = true;
}

rs_context::~rs_context()
{
    assert(singleton_alive);
    singleton_alive = false;
}
