// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include <mutex>
#include <array>
#include <string>

#include "r200.h"
#include "f200.h"
#include "sr300.h"
#include "zr300.h"
#include "uvc.h"
#include "context.h"


#define constexpr_support 1

#ifdef _MSC_VER
#if (_MSC_VER <= 1800) // constexpr is not supported in MSVC2013
#undef constexpr_support
#define constexpr_support 0
#endif
#endif

#if (constexpr_support == 1)
template<unsigned... Is> struct seq{};
template<unsigned N, unsigned... Is>
struct gen_seq : gen_seq<N-1, N-1, Is...>{};
template<unsigned... Is>
struct gen_seq<0, Is...> : seq<Is...>{};

template<unsigned N1, unsigned... I1, unsigned N2, unsigned... I2>
constexpr std::array<char const, N1+N2-1> concat(char const (&a1)[N1], char const (&a2)[N2], seq<I1...>, seq<I2...>){
  return {{ a1[I1]..., a2[I2]... }};
}

template<unsigned N1, unsigned N2>
constexpr std::array<char const, N1+N2-1> concat(char const (&a1)[N1], char const (&a2)[N2]){
  return concat(a1, a2, gen_seq<N1-1>{}, gen_seq<N2>{});
}

constexpr auto rs_api_version = concat("VERSION: ",RS_API_VERSION_STR);

#else    // manual version tracking is required
static const std::string rs_api_version("VERSION: 1.11.1");

#endif

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/usbdevice_fs.h>
#include <linux/usb/ch9.h>
#include "ds-device.h"

namespace rsimpl { namespace uvc
{
    std::string get_usb_hub_name(const device & device);
}}

using namespace rsimpl::uvc;

const int USB_PORT_FEAT_POWER   = 8;
const int USB_HUB_TIMEOUT       = 5000;

static void set_hub_power(device &device, bool enable, int timeout)
{
    std::string filename = get_usb_hub_name(device);
    int fd = open(filename.c_str(), O_RDWR);

    usbdevfs_ctrltransfer ctrl;

    ctrl.bRequestType = USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_OTHER;
    ctrl.wValue = USB_PORT_FEAT_POWER;
    ctrl.wIndex = 1;//portnum;
    ctrl.wLength = 0;
    ctrl.timeout = USB_HUB_TIMEOUT;
    ctrl.data = NULL;
    ctrl.bRequest = enable ? USB_REQ_SET_FEATURE : USB_REQ_CLEAR_FEATURE;

    ioctl(fd, USBDEVFS_CONTROL, &ctrl);

    usleep( timeout * 1000 );
}

bool is_compatible(std::shared_ptr<rs_device> device)
{
    return device->supports(RS_CAPABILITIES_ENUMERATION);
}

rs_context_base::rs_context_base()
{
    context = rsimpl::uvc::create_context();

    try
    {
        create_devices();
    }
    catch(std::shared_ptr<device> device)
    {
        LOG_WARNING("Failed to read camera info - cycling hub power");

        set_hub_power(*device, false, 1500);
        set_hub_power(*device, true, 8500);

        create_devices();
    }
}

void rs_context_base::create_devices()
{
    devices.clear();

    for(auto device : query_devices(context))
    {
        try
        {
            LOG_INFO("UVC device detected with VID = 0x" << std::hex << get_vendor_id(*device) << " PID = 0x" << get_product_id(*device));

            if (get_vendor_id(*device) != VID_INTEL_CAMERA)
                continue;

            std::shared_ptr<rs_device> rs_dev;

            switch(get_product_id(*device))
            {
                case R200_PRODUCT_ID:  rs_dev = rsimpl::make_r200_device(device); break;
                case LR200_PRODUCT_ID: rs_dev = rsimpl::make_lr200_device(device); break;
                case ZR300_PRODUCT_ID: rs_dev = rsimpl::make_zr300_device(device); break;
                case F200_PRODUCT_ID:  rs_dev = rsimpl::make_f200_device(device); break;
                case SR300_PRODUCT_ID: rs_dev = rsimpl::make_sr300_device(device); break;
            }

            if (rs_dev && is_compatible(rs_dev))
            {
                devices.push_back(rs_dev);
            }
            else
            {
                LOG_ERROR("Device is not supported by librealsense!");
            }
        }
        catch(...)
        {
            throw device;
        }
    }
}

// Enforce singleton semantics on rs_context
rs_context* rs_context_base::instance = nullptr;
int rs_context_base::ref_count = 0;
std::mutex rs_context_base::instance_lock;
std::string rs_context_base::api_version = std::string(rs_api_version.begin(),rs_api_version.end());

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

size_t rs_context_base::get_device_count() const
{
    return devices.size();
}

rs_device* rs_context_base::get_device(int index) const
{
    return devices[index].get();
}
