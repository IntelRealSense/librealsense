// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "context.h"
#include "uvc.h"
#include "r200.h"
#include "f200.h"
#include <mutex>

rs_context_base::rs_context_base()
{
    context = rsimpl::uvc::create_context();

    for(auto device : query_devices(context))
    {
        LOG_INFO("UVC device detected with VID = 0x" << std::hex << get_vendor_id(*device) << " PID = 0x" << get_product_id(*device));

        if (get_vendor_id(*device) != PID_INTEL_CAMERA)
            continue;

        switch(get_product_id(*device))
        {
            case R200_PRODUCT_ID: devices.push_back(rsimpl::make_r200_device(device)); break;
            case LR200_PRODUCT_ID: devices.push_back(rsimpl::make_lr200_device(device)); break;
            case ZR300_PRODUCT_ID: devices.push_back(rsimpl::make_zr300_device(device)); break;
            case F200_PRODUCT_ID: devices.push_back(rsimpl::make_f200_device(device)); break;
            case SR300_PRODUCT_ID: devices.push_back(rsimpl::make_sr300_device(device)); break;
        }
    }
}

// Enforce singleton semantics on rs_context
rs_context* rs_context_base::instance = nullptr;
int rs_context_base::ref_count = 0;
std::mutex rs_context_base::instance_lock;

rs_context* rs_context_base::acquire_instance()
{
    std::lock_guard<std::mutex> lock(instance_lock);
    if (ref_count++ == 0)
    {
        instance = new rs_context_base();
    }
    return instance;
}

void rs_context_base::release_instance()
{
    std::lock_guard<std::mutex> lock(instance_lock);
    if (--ref_count == 0)
    {
        delete instance;
    }
}

rs_context_base::~rs_context_base()
{
    assert(ref_count == 0);
}

int rs_context_base::get_device_count() const
{
    return devices.size();
}

rs_device* rs_context_base::get_device(int index) const
{
    return devices[index].get();
}