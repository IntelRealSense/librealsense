#include "../src/uvc.h"

#include <map>
#include <algorithm>
#include <iostream>

namespace uvc = rsimpl::uvc;

struct smart_context
{
    std::shared_ptr<uvc::context> ctx;
    std::map<std::string, std::shared_ptr<uvc::device>> id_to_device;

    smart_context() : ctx(uvc::create_context()) {}

    void list() const
    {
        for(auto & p : id_to_device)
        {
            std::cout << p.first << ": ";
            if(!!p.second) std::cout << "connected, vid = " << get_vendor_id(*p.second) << ", pid = " << get_product_id(*p.second) << std::endl;
            else std::cout << "disconnected" << std::endl;
        }
    }

    void refresh()
    {
        auto device_list = query_devices(ctx);
        for(auto & d : device_list)
        {
            auto it = id_to_device.find(get_unique_id(*d));
            if(it == end(id_to_device))
            {
                std::cout << "Discovered new device " << get_unique_id(*d) << std::endl;
                id_to_device.insert({get_unique_id(*d), d});
            }
            else if(!it->second)
            {
                std::cout << "\nReconnected to device " << get_unique_id(*d) << std::endl;
                it->second = d;
            }
            else
            {
                // Found an existing device, ignore it
            }
        }
        
        for(auto & p : id_to_device)
        {
            if(!p.second) continue;
            auto it = std::find_if(begin(device_list), end(device_list), [p](const std::shared_ptr<uvc::device> & d) { return p.first == get_unique_id(*d); }); 
            if(it == end(device_list))
            {
                std::cout << "\nLost connection to device " << p.first << std::endl;
                p.second = nullptr;
            }
        }
    }

    void reset(std::string id)
    {
        auto it = id_to_device.find(id);
        if(it == end(id_to_device)) return;

        const uvc::guid R200_LEFT_RIGHT_XU = {0x18682d34, 0xdd2c, 0x4073, {0xad, 0x23, 0x72, 0x14, 0x73, 0x9a, 0x07, 0x4c}};
        uint8_t reset = 1;

        switch(get_product_id(*it->second))
        {
        case 0xA80:
            init_controls(*it->second, 0, R200_LEFT_RIGHT_XU);
            set_control(*it->second, 0, 16, &reset, sizeof(reset));
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        refresh();

        while(!it->second)
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

    int i=0;
    for(auto & p : ctx.id_to_device)
    {
        uvc::set_subdevice_mode(*p.second, 1, 628, 469, 'Z16 ', 60, [i](const void * frame) { std::cout << i; });
        uvc::start_streaming(*p.second, 0);
        ++i;
    }
    std::this_thread::sleep_for(std::chrono::seconds(5));

    i=0;
    for(auto & p : ctx.id_to_device)
    {
        ctx.reset(p.first);

        uvc::set_subdevice_mode(*p.second, 1, 628, 469, 'Z16 ', 60, [i](const void * frame) { std::cout << i; });
        uvc::start_streaming(*p.second, 0);
        ++i;

        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
    return EXIT_SUCCESS;
}
catch(const std::exception & e)
{
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}