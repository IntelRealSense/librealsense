// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "catch/catch.hpp"
#include "usb/usb-enumerator.h"
#include "usb/usb-device.h"
#include "hw-monitor.h"
#include "librealsense2/h/rs_option.h"
#include <map>

using namespace librealsense;
using namespace librealsense::platform;

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

TEST_CASE("query_devices", "[live][usb]") 
{
    auto  devices = usb_enumerator::query_devices();

    REQUIRE(devices.size());

    switch (devices.size())
    {
        case 0: printf("\nno USB device found\n\n"); break;
        case 1: printf("\n1 USB device found:\n\n"); break;
        default:
            printf("\n%d USB devices found:\n\n", devices.size()); break;
    }

    int device_counter = 0;
    for (auto&& dev : devices)
    {
        auto device_info = dev->get_info();
        printf("%d)\tid: %s\n", ++device_counter, device_info.id.c_str());

        auto subdevices = dev->get_subdevices_infos();
        for (auto&& subdevice_info : subdevices)
        {            
            printf("\tmi: %d\n", subdevice_info.mi);
        }
    }
    printf("===============================================================================\n");
}

TEST_CASE("is_device_connected", "[live][usb]")
{
    auto  devices = usb_enumerator::query_devices();
    int device_counter = 0;
    for (auto&& dev : devices)
    {
        auto sts = usb_enumerator::is_device_connected(dev);
        REQUIRE(sts);
    }
}

TEST_CASE("filter_subdevices", "[live][usb]")
{
    auto  devices = usb_enumerator::query_devices();
    int device_counter = 0;
    std::map<usb_subclass, std::vector<rs_usb_interface>> interfaces;
    for (auto&& dev : devices)
    {
        interfaces[USB_SUBCLASS_CONTROL] = dev->get_interfaces(USB_SUBCLASS_CONTROL);
        interfaces[USB_SUBCLASS_STREAMING] = dev->get_interfaces(USB_SUBCLASS_STREAMING);
        interfaces[USB_SUBCLASS_HWM] = dev->get_interfaces(USB_SUBCLASS_HWM);
        auto any = dev->get_interfaces(USB_SUBCLASS_ANY);

        auto count = interfaces.at(USB_SUBCLASS_CONTROL).size() +
            interfaces.at(USB_SUBCLASS_STREAMING).size() +
            interfaces.at(USB_SUBCLASS_HWM).size();
        REQUIRE(any.size() == count);

        for (auto&& intfs : interfaces)
        {
            for (auto&& intf : intfs.second)
            {
                REQUIRE(intf->get_subclass() == intfs.first);
            }
        }
    }
}

TEST_CASE("first_endpoints_direction", "[live][usb]")
{
    auto  devices = usb_enumerator::query_devices();
    int device_counter = 0;
    for (auto&& dev : devices)
    {
        auto interfaces = dev->get_interfaces(USB_SUBCLASS_HWM);

        for (auto&& i : interfaces)
        {
            auto w = i->first_endpoint(USB_ENDPOINT_DIRECTION_WRITE);
            if(w)
                REQUIRE(USB_ENDPOINT_DIRECTION_WRITE == w->get_direction());
            auto r = i->first_endpoint(USB_ENDPOINT_DIRECTION_READ);
            if(r)
                REQUIRE(USB_ENDPOINT_DIRECTION_READ == r->get_direction());
        }
    }
}

//control transfer test
TEST_CASE("query_controls", "[live][usb]")
{
    auto  devices = usb_enumerator::query_devices();

    const int REQ_TYPE_GET = 0xa1;

    std::vector<int> requests =
    {
        uvc_req_code.at("UVC_GET_CUR"),
        uvc_req_code.at("UVC_GET_MIN"),
        uvc_req_code.at("UVC_GET_MAX"),
        uvc_req_code.at("UVC_GET_DEF")
    };

    bool controls_found = false;

    for (auto&& dev : devices)
    {
        auto m = dev->open();

        std::vector<rs_usb_interface> ctrl_interfaces = dev->get_interfaces(USB_SUBCLASS_CONTROL);

        //uvc units, rs uvc api is not implemented yet so we use hard coded mapping for reading the PUs
        std::map<int,int> processing_units =
        {
          {0,2},
          {3,7}
        };

        int timeout = 1000;

        for (auto&& intf : ctrl_interfaces)
        {
            controls_found = true;
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
                    auto sts = m->control_transfer(REQ_TYPE_GET, req, value, index, reinterpret_cast<uint8_t*>(&val), sizeof(val), timeout);
                    if(sts <= 0)
                        break;
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
        printf("There are no control interfaces available, force winusb/libuvc to probe controls\n");
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

//bulk transfer test
TEST_CASE("read_gvd", "[live][usb]")
{
    auto  devices = usb_enumerator::query_devices();
    int device_counter = 0;

    printf("Devices FW version:\n");
    for (auto&& dev : devices)
    {
        command_transfer_usb ctu(dev);

        bool is_sr300 = dev->get_info().pid == 0x0AA5 ? true : false;

        int timeout = 1000;
        bool require_response = true;
        auto data = create_gvd_request_buffer(is_sr300);

        auto res = ctu.send_receive(data, timeout, require_response);
        REQUIRE(res.size());

        uint8_t fws[8];
        uint16_t header_size = 4;
        int fw_version_offset = is_sr300 ? 0 : 12;

        librealsense::copy(fws, res.data() + header_size + fw_version_offset, 8);
        std::string fw = to_string() << static_cast<int>(fws[3]) << "." << static_cast<int>(fws[2])
            << "." << static_cast<int>(fws[1]) << "." << static_cast<int>(fws[0]);

        printf("device: %s, fw: %s\n", dev->get_info().id.c_str(), fw.c_str());
    }
    printf("===============================================================================\n");
}
