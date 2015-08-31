#include "context.h"
#include "uvc.h"
#include "r200.h"
#include "f200.h"

#include <thread>

rs_context::rs_context()
{
    uvc_error_t status = rsimpl::uvc::init(&context, NULL);
    if (status < 0)
    {
        rsimpl::uvc::perror(status, "uvc_init");
        throw std::runtime_error("Could not initialize UVC context");
    }

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

    uvc_error_t status = rsimpl::uvc::get_device_list(context, &list);
    if (status != UVC_SUCCESS)
    {
        rsimpl::uvc::perror(status, "uvc_get_device_list");
        return;
    }

    size_t index = 0;
    while (list[index] != nullptr)
    {
        auto dev = list[index];

        uvc_device_descriptor_t * desc;
        rsimpl::uvc::get_device_descriptor(dev, &desc);
        switch(desc->idProduct)
        {
        case 2688: cameras.push_back(std::make_shared<rsimpl::r200_camera>(context, list[index])); break;
        case 2662: cameras.push_back(std::make_shared<rsimpl::f200_camera>(context, list[index])); break;
        case 2725: throw std::runtime_error("IVCAM 1.5 / SR300 is not supported at this time");
        }
        rsimpl::uvc::free_device_descriptor(desc);

        ++index;
    }

    rsimpl::uvc::free_device_list(list, 1);
}
