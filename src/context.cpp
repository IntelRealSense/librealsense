#include "context.h"
#include "uvc.h"
#include "r200.h"
#include "f200.h"

#include <thread>

rs_context::rs_context()
{
    rsimpl::uvc::init(&context, NULL);
    query_device_list();
}

rs_context::~rs_context()
{
    cameras.clear(); // tear down cameras before context
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    if (context) rsimpl::uvc::exit(context);
}

void rs_context::query_device_list()
{
    uvc_device_t **list;
    rsimpl::uvc::get_device_list(context, &list);

    size_t index = 0;
    while (list[index] != nullptr)
    {
        rsimpl::uvc::device device(list[index++]);
        switch(device.get_product_id())
        {
        case 2688: cameras.push_back(std::make_shared<rsimpl::r200_camera>(context, device)); break;
        case 2662: cameras.push_back(std::make_shared<rsimpl::f200_camera>(context, device)); break;
        case 2725: throw std::runtime_error("IVCAM 1.5 / SR300 is not supported at this time");
        }
    }

    rsimpl::uvc::free_device_list(list, 1);
}
