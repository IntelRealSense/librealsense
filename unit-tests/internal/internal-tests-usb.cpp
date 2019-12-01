// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "catch/catch.hpp"
#include "usb/usb-enumerator.h"
#include "usb/usb-device.h"
#include "hw-monitor.h"
#include "librealsense2/h/rs_option.h"
#include <map>

using namespace librealsense::platform;

#define USB_SUBCLASS_CONTROL 1

static std::map< std::string, int> uvc_req_code = {
    {"UVC_RC_UNDEFINED", 0x00},
    {"UVC_SET_CUR", 0x01},
    {"UVC_GET_CUR", 0x81},
    {"UVC_GET_MIN", 0x82},
    {"UVC_GET_MAX", 0x83},
    {"UVC_GET_RES", 0x84},
    {"UVC_GET_LEN", 0x85},
    {"UVC_GET_INFO", 0x86},
    {"UVC_GET_DEF", 0x87},
    {"UVC_REQ_TYPE_GET", 0xa1},
    {"UVC_REQ_TYPE_SET", 0x21}
};

std::string req_to_string(int req)
{
    auto res = std::find_if(std::begin(uvc_req_code), std::end(uvc_req_code), [&](const std::pair<std::string, int> &p)
    {
        return p.second == req;
    });
    if (res != std::end(uvc_req_code))
        return res->first;
    return "";
}

static std::map< std::string, int> pu_controls = {
    {"PU_CONTROL_UNDEFINED", 0x00},
    {"PU_BACKLIGHT_COMPENSATION_CONTROL", 0x01},
    {"PU_BRIGHTNESS_CONTROL", 0x02},
    {"PU_CONTRAST_CONTROL", 0x03 },
    {"PU_GAIN_CONTROL", 0x04 },
    {"PU_POWER_LINE_FREQUENCY_CONTROL", 0x05 },
    {"PU_HUE_CONTROL", 0x06 },
    {"PU_SATURATION_CONTROL", 0x07 },
    {"PU_SHARPNESS_CONTROL", 0x08 },
    {"PU_GAMMA_CONTROL", 0x09 },
    {"PU_WHITE_BALANCE_TEMPERATURE_CONTROL", 0x0A },
    {"PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL", 0x0B },
    {"PU_WHITE_BALANCE_COMPONENT_CONTROL", 0x0C },
    {"PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL", 0x0D },
    {"PU_DIGITAL_MULTIPLIER_CONTROL", 0x0E },
    {"PU_DIGITAL_MULTIPLIER_LIMIT_CONTROL", 0x0F },
    {"PU_HUE_AUTO_CONTROL", 0x10 },
    {"PU_ANALOG_VIDEO_STANDARD_CONTROL", 0x11 },
    {"PU_ANALOG_LOCK_STATUS_CONTROL", 0x12 },
    {"PU_CONTRAST_AUTO_CONTROL", 0x13 },

};

std::vector<usb_device_info> get_devices_info()
{
    std::vector<usb_device_info> rv;
    auto  devices_info = usb_enumerator::query_devices_info();
    for (auto&& info : devices_info)
    {
        if (std::find_if(rv.begin(), rv.end(), [&](const usb_device_info& i) { return i.id == info.id; }) == rv.end())
            rv.push_back(info);
    }
    return rv;
}

TEST_CASE("query_devices", "[live][usb]") 
{
    auto  devices_info = usb_enumerator::query_devices_info();

    REQUIRE(devices_info.size());

    if (devices_info.size() == 1)
        printf("\n1 USB device found:\n\n");
    else
        ("\n%d USB devices found:\n\n", devices_info.size());

    int device_counter = 0;
    for (auto&& device_info : devices_info)
    {
        printf("%d)uid: %s\tmi: %d\tpath: %s\n", ++device_counter, device_info.unique_id.c_str(), device_info.mi, device_info.id.c_str());
    }
    printf("===============================================================================\n");
}

TEST_CASE("first_endpoints_direction", "[live][usb]")
{
    auto devices_info = get_devices_info();
    int device_counter = 0;
    for (auto&& info : devices_info)
    {
        if(info.vid != 0x8086)
            continue;
        auto dev = usb_enumerator::create_usb_device(info);
        if (!dev)
            continue;
        auto interfaces = dev->get_interfaces();
        auto it = std::find_if(interfaces.begin(), interfaces.end(),
            [](const rs_usb_interface& i) { return i->get_class() == RS2_USB_CLASS_VENDOR_SPECIFIC; });

        REQUIRE(it != interfaces.end());

        auto hwm = *it;
        auto w = hwm->first_endpoint(RS2_USB_ENDPOINT_DIRECTION_WRITE);
        if (w)
            REQUIRE(RS2_USB_ENDPOINT_DIRECTION_WRITE == w->get_direction());
        auto r = hwm->first_endpoint(RS2_USB_ENDPOINT_DIRECTION_READ);
        if (r)
            REQUIRE(RS2_USB_ENDPOINT_DIRECTION_READ == r->get_direction());
    }
}

//control transfer test
TEST_CASE("query_controls", "[live][usb]")
{
    auto devices_info = get_devices_info();

    const int REQ_TYPE_GET = 0xa1;

    std::vector<int> requests =
    {
        uvc_req_code.at("UVC_GET_CUR"),
        uvc_req_code.at("UVC_GET_MIN"),
        uvc_req_code.at("UVC_GET_MAX"),
        uvc_req_code.at("UVC_GET_DEF")
    };

    bool controls_found = false;

    for (auto&& info : devices_info)
    {
        if(info.vid != 0x8086)
            continue;
        auto dev = usb_enumerator::create_usb_device(info);
        if (!dev)
            continue;
        if (0 != info.mi)        // Lookup for controls interface only
            continue;
        auto m = dev->open(info.mi);

        std::vector<rs_usb_interface> interfaces = dev->get_interfaces();
        REQUIRE(interfaces.size() > 0);

        //uvc units, rs uvc api is not implemented yet so we use hard coded mapping for reading the PUs
        std::map<int,int> processing_units =
        {
          {0,2},
          {2,2},
          {3,7}
        };

        int timeout = 1000;

        for (auto&& intf : interfaces)
        {
            if (intf->get_class() != RS2_USB_CLASS_VIDEO || intf->get_subclass() != USB_SUBCLASS_CONTROL)
                continue;

            controls_found = true;
            if (processing_units.find(intf->get_number()) == processing_units.end())
                continue;

            auto unit = processing_units.at(intf->get_number());

            printf("interface: %d, processing unit: %d\n", intf->get_number(), unit);
            int index = unit << 8 | intf->get_number();


            for(auto&& ctrl : pu_controls)
            {
                int value = ctrl.second << 8;
                std::map<int,int> values;
                for(auto&& req : requests)
                {
                    int val = 0;
                    uint32_t transferred = 0;
                    auto sts = m->control_transfer(REQ_TYPE_GET, req, value, index, reinterpret_cast<uint8_t*>(&val), sizeof(val), transferred, timeout);
                    if (sts != RS2_USB_STATUS_SUCCESS)
                        continue;
                    REQUIRE(transferred == sizeof(val));
                    values[req] = val;
                }

                if(values.size() > 0)
                    printf("%s:\n", ctrl.first.c_str());

                for(auto&& req : values)
                {
                    printf("%s:\t%d\t", req_to_string(req.first).c_str(), req.second);
                }

                if(values.size() > 0)
                    printf("\n");
            }
        }
    }
    if (!controls_found)
        printf("There are no control interfaces available, force winusb to probe controls\n");
    printf("===============================================================================\n");
}

std::vector<uint8_t> create_gvd_request_buffer(bool is_sr300)
{
    uint16_t header_size = 4;
    uint16_t data_size = 20;
    std::vector<uint8_t> rv(header_size + data_size);

    uint32_t GVD = is_sr300 ? 0x3B : 0x10;

    uint16_t IVCAM_MONITOR_MAGIC_NUMBER = 0xcdab;

    memcpy(rv.data(), &data_size, sizeof(uint16_t));
    memcpy(rv.data() + 2, &IVCAM_MONITOR_MAGIC_NUMBER, sizeof(uint16_t));
    memcpy(rv.data() + 4, &GVD, sizeof(uint32_t));

    return rv;
}

void read_gvd(const rs_usb_device& dev)
{
    command_transfer_usb ctu(dev);

    bool is_sr300 = ((dev->get_info().pid == 0x0AA5) || (dev->get_info().pid == 0x0B48)) ? true : false;

    int timeout = 100;
    bool require_response = true;
    auto data = create_gvd_request_buffer(is_sr300);

    auto res = ctu.send_receive(data, timeout, require_response);
    REQUIRE(res.size());

    uint8_t fws[8];
    uint16_t header_size = 4;
    int fw_version_offset = is_sr300 ? 0 : 12;

    librealsense::copy(fws, res.data() + header_size + fw_version_offset, 8);
    std::string fw = librealsense::to_string() << static_cast<int>(fws[3]) << "." << static_cast<int>(fws[2])
        << "." << static_cast<int>(fws[1]) << "." << static_cast<int>(fws[0]);

    printf("device: %s, fw: %s\n", dev->get_info().unique_id.c_str(), fw.c_str());
}

//bulk transfer test
TEST_CASE("read_gvd", "[live][usb]")
{
    auto devices_info = get_devices_info();
    int device_counter = 0;

    printf("Devices FW version:\n");
    for (auto&& info : devices_info)
    {

        if(info.vid != 0x8086)
            continue;
        auto dev = usb_enumerator::create_usb_device(info);
        if (!dev)
            continue;
        for(int i = 0; i < 3; i++)
            read_gvd(dev);
    }
    printf("===============================================================================\n");
}

TEST_CASE("query_devices_info_duration", "[live][usb]")
{
    auto begin = std::chrono::system_clock::now();
    auto devices_info = usb_enumerator::query_devices_info();
    auto end = std::chrono::system_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
    REQUIRE(diff < 500);
    printf("===============================================================================\n");
}

TEST_CASE("create_device_duration", "[live][usb]")
{
    auto devices_info = get_devices_info();
    int device_counter = 0;
    for (auto&& info : devices_info)
    {
        if (info.vid != 0x8086)
            continue;
        auto begin = std::chrono::system_clock::now();
        auto dev = usb_enumerator::create_usb_device(info);
        auto end = std::chrono::system_clock::now();
        auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
        REQUIRE(diff < 500);
        if (!dev)
            continue;
    }
    printf("===============================================================================\n");
}
