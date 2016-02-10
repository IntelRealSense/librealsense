#include "../src/uvc.h"

#include <map>
#include <algorithm>
#include <iostream>

namespace uvc = rsimpl::uvc;

struct smart_device
{
    std::string unique_id;
    std::shared_ptr<uvc::device> device;

    bool is_connected() const { return !!device; }
};

struct smart_context
{
    std::shared_ptr<uvc::context> ctx;
    std::vector<smart_device> devices;

    smart_context() : ctx(uvc::create_context()) {}

    void list() const
    {
        for(auto & d : devices)
        {
            std::cout << d.unique_id << ": ";
            if(d.is_connected()) std::cout << "connected, vid = " << get_vendor_id(*d.device) << ", pid = " << get_product_id(*d.device) << std::endl;
            else std::cout << "disconnected" << std::endl;
        }
    }

    void refresh()
    {
        auto device_list = query_devices(ctx);
        for(auto & d : device_list)
        {
            auto it = std::find_if(begin(devices), end(devices), [d](const smart_device & s) { return s.unique_id == get_unique_id(*d); });
            if(it == end(devices))
            {
                std::cout << "Discovered new device " << get_unique_id(*d) << std::endl;
                devices.push_back({get_unique_id(*d), d});
            }
            else if(!it->device)
            {
                std::cout << "\nReconnected to device " << get_unique_id(*d) << std::endl;
                it->device = d;
            }
            else
            {
                // Found an existing device, ignore it
            }
        }
        
        for(auto & s : devices)
        {
            if(!s.device) continue;
            auto it = std::find_if(begin(device_list), end(device_list), [s](const std::shared_ptr<uvc::device> & d) { return s.unique_id == get_unique_id(*d); }); 
            if(it == end(device_list))
            {
                std::cout << "\nLost connection to device " << s.unique_id << std::endl;
                s.device = nullptr;
            }
        }
    }

    void reset(int index)
    {
        const uvc::guid R200_LEFT_RIGHT_XU = {0x18682d34, 0xdd2c, 0x4073, {0xad, 0x23, 0x72, 0x14, 0x73, 0x9a, 0x07, 0x4c}};
        uint8_t reset = 1;

        switch(get_product_id(*devices[index].device))
        {
        case 0xA80:
            init_controls(*devices[index].device, 0, R200_LEFT_RIGHT_XU);
            set_control(*devices[index].device, 0, 16, &reset, sizeof(reset));
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        refresh();

        while(!devices[index].device)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            refresh();
        }
    }
};

int main() try
{
    smart_context ctx;

    ctx.refresh();
    ctx.list();

    for(int i=0; i<ctx.devices.size(); ++i)
    {
        uvc::set_subdevice_mode(*ctx.devices[i].device, 1, 628, 469, 'Z16 ', 60, [i](const void * frame) { std::cout << i; });
        uvc::start_streaming(*ctx.devices[i].device, 0);
    }
    std::this_thread::sleep_for(std::chrono::seconds(5));

    for(int i=0; i<ctx.devices.size(); ++i)
    {
        ctx.reset(i);

        uvc::set_subdevice_mode(*ctx.devices[i].device, 1, 628, 469, 'Z16 ', 60, [i](const void * frame) { std::cout << i; });
        uvc::start_streaming(*ctx.devices[i].device, 0);

        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
    return EXIT_SUCCESS;
}
catch(const std::exception & e)
{
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}