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
        case 2688: devices.push_back(std::make_shared<rsimpl::r200_camera>(device)); break;
        case 2662: devices.push_back(std::make_shared<rsimpl::f200_camera>(device, false)); break;
        case 2725: devices.push_back(std::make_shared<rsimpl::f200_camera>(device, true)); break; // SR300
        }
    }
}

