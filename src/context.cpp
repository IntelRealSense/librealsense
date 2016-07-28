// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "context.h"
#include "uvc.h"
#include "r200.h"
#include "f200.h"
#include <mutex>

rs_context::rs_context()
{
    context = rsimpl::uvc::create_context();

    for(auto device : query_devices(context))
    {
        LOG_INFO("UVC device detected with VID = 0x" << std::hex << get_vendor_id(*device) << " PID = 0x" << get_product_id(*device));
        LOG_INFO("USB Port number =" << get_usb_port_id(*device));

		if (get_vendor_id(*device) != 32902)
			continue;
				
        switch(get_product_id(*device))
        {
        case 2688: devices.push_back(rsimpl::make_r200_device(device)); break;
        case 2662: devices.push_back(rsimpl::make_f200_device(device)); break;
        case 2725: devices.push_back(rsimpl::make_sr300_device(device)); break;
        }
    }
}

// Enforce singleton semantics on rs_context
rs_context* rs_context::instance = nullptr;
int rs_context::ref_count = 0;
std::mutex rs_context::instance_lock;

rs_context* rs_context::acquire_instance()
{
    std::lock_guard<std::mutex> lock(instance_lock);
    if (ref_count++ == 0)
    {
        instance = new rs_context();
    }
    return instance;
}

void rs_context::release_instance()
{
    std::lock_guard<std::mutex> lock(instance_lock);
    if (--ref_count == 0)
    {
        delete instance;
    }
}

rs_context::~rs_context()
{
    assert(ref_count == 0);
}
