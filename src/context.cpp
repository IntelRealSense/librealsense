#include "context.h"
#include "uvc.h"
#include "r200.h"
#include "f200.h"

#include <thread>

rs_context::rs_context()
{
    context = rsimpl::uvc::context::create();
    for(auto device : context.query_devices())
    {
        switch(device.get_product_id())
        {
        case 2688: cameras.push_back(std::make_shared<rsimpl::r200_camera>(device)); break;
        case 2662: cameras.push_back(std::make_shared<rsimpl::f200_camera>(device)); break;
        case 2725: throw std::runtime_error("IVCAM 1.5 / SR300 is not supported at this time");
        }
    }
}

rs_context::~rs_context()
{
    cameras.clear(); // tear down cameras before context
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}
