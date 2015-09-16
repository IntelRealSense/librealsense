#include "context.h"
#include "uvc.h"
#include "r200.h"
#include "f200.h"

rs_context::rs_context() : context(rsimpl::uvc::context::create())
{
    for(auto device : context.query_devices())
    {
        switch(device.get_product_id())
        {
        case 2688: devices.push_back(rsimpl::make_r200_device(device)); break;
        case 2662: devices.push_back(rsimpl::make_f200_device(device)); break;
        case 2725: devices.push_back(rsimpl::make_sr300_device(device)); break;
        }
    }
}

