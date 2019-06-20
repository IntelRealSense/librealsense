// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#if (_MSC_FULL_VER < 180031101)
#error At least Visual Studio 2013 Update 4 is required to compile this backend
#endif

#include "usb/usb-enumerator.h"
#include "device-winusb.h"
#include "interface-winusb.h"
#include "win/win-helpers.h"
#include "types.h"

#include <atlstr.h>
#include <Windows.h>
#include <Sddl.h>
#include <string>
#include <regex>
#include <sstream>
#include <mutex>

#include <Cfgmgr32.h>
#include <SetupAPI.h>

#pragma comment(lib, "winusb.lib")

namespace librealsense
{
    namespace platform
    {
        //https://docs.microsoft.com/en-us/windows-hardware/drivers/usbcon/supported-usb-classes#microsoft-provided-usb-device-class-drivers
        const std::map<std::string, usb_class> guids = {
            {"{175695CD-30D9-4F87-8BE3-5A8270F49A31}", RS2_USB_CLASS_VENDOR_SPECIFIC}, //Ivcam
            {"{08090549-CE78-41DC-A0FB-1BD66694BB0C}", RS2_USB_CLASS_VENDOR_SPECIFIC},  //D4xx
            {"{a5dcbf10-6530-11d2-901f-00c04fb951ed}", RS2_USB_CLASS_UNSPECIFIED},  // for DFU
            {"{ca3e7ab9-b4c3-4ae6-8251-579ef933890f}", RS2_USB_CLASS_VIDEO}, // win 10
            {"{50537bc3-2919-452d-88a9-b13bbf7d2459}", RS2_USB_CLASS_VIDEO}, // win 7
        };

        std::vector<std::wstring> query_by_interface(GUID guid)
        {
            std::vector<std::wstring> rv;

            CONFIGRET cr = CR_SUCCESS;
            ULONG length = 0;

            do {
                cr = CM_Get_Device_Interface_List_Size(&length, (LPGUID)&guid, NULL,
                    CM_GET_DEVICE_INTERFACE_LIST_PRESENT);

                if (cr != CR_SUCCESS)
                    break;

                std::vector<WCHAR> device_list(length);
                cr = CM_Get_Device_Interface_List((LPGUID)&guid, NULL, device_list.data(), length,
                    CM_GET_DEVICE_INTERFACE_LIST_PRESENT);

                int offset = 0;
                while (offset < length)
                {
                    auto str = std::wstring((device_list.data() + offset));
                    if(!str.empty())
                        rv.push_back(str);
                    offset += str.size() + 1;
                }
            } while (cr == CR_BUFFER_SMALL);

            return rv;
        }

        std::vector<std::wstring> query_by_interface(const std::string& guidStr)
        {
            GUID guid;
            std::wstring guidWStr(guidStr.begin(), guidStr.end());
            CHECK_HR(CLSIDFromString(guidWStr.c_str(), static_cast<LPCLSID>(&guid)));
            return query_by_interface(guid);
        }

        usb_device_info get_info(const std::wstring device_wstr)
        {
            usb_device_info rv{};
            std::smatch matches;
            std::string device_str(device_wstr.begin(), device_wstr.end());

            std::regex regex_camera_interface("\\b(.*VID_)(.*)(&PID_)(.*)(&MI_)(.*)(#.*&)(.*)(&.*)(&.*)", std::regex_constants::icase);
            std::regex regex_usb_interface("\\b(.*VID_)(.*)(&PID_)(.*)(#.*&)(.*)(&.*)(&.*)", std::regex_constants::icase);
            std::regex regex_dfu_interface("\\b(.*VID_)(.*)(&PID_)(.*)(#)(.*)(#)(.*)", std::regex_constants::icase);

            if (std::regex_search(device_str, matches, regex_camera_interface) && matches.size() == 11)
            {
                rv.id = matches[0];
                std::stringstream vid; vid << std::hex << matches[2]; vid >> rv.vid;
                std::stringstream pid; pid << std::hex << matches[4]; pid >> rv.pid;
                std::stringstream mi; mi << std::hex << matches[6]; mi >> rv.mi;
                std::stringstream uid; uid << std::hex << matches[8]; uid >> rv.unique_id;
                return rv;
            }
            if (std::regex_search(device_str, matches, regex_usb_interface) && matches.size() == 9)
            {
                rv.id = matches[0];
                std::stringstream vid; vid << std::hex << matches[2]; vid >> rv.vid;
                std::stringstream pid; pid << std::hex << matches[4]; pid >> rv.pid;
                std::stringstream uid; uid << std::hex << matches[6]; uid >> rv.unique_id;
                return rv;
            }
            if (std::regex_search(device_str, matches, regex_dfu_interface))
            {
                rv.id = matches[0];
                std::stringstream vid; vid << std::hex << matches[2]; vid >> rv.vid;
                std::stringstream pid; pid << std::hex << matches[4]; pid >> rv.pid;
                std::stringstream uid; uid << std::hex << matches[6]; uid >> rv.unique_id;
                return rv;
            }

            return rv;
        }

        std::vector<usb_device_info> usb_enumerator::query_devices_info()
        {
            std::vector<usb_device_info> rv;

            for (auto&& guid : guids)
            {
                for (auto&& id : query_by_interface(guid.first))
                {
                    auto info = get_info(id.c_str());
                    if (info.vid == 0) //unsupported device
                        continue;
                    info.cls = guid.second;
                    rv.push_back(info);
                }
            }
            return rv;
        }

        rs_usb_device usb_enumerator::create_usb_device(const usb_device_info& info)
        {
            std::vector<std::wstring> devices_path;

            for (auto&& guid : guids)
            {
                for (auto&& id : query_by_interface(guid.first))
                {
                    auto i = get_info(id.c_str());
                    if (i.vid == info.vid && i.pid == info.pid && i.unique_id == info.unique_id)
                        devices_path.push_back(id);
                }
            }

            if (devices_path.size() == 0)
            {
                LOG_WARNING("failed to locate usb interfaces for device: " << info.id);
                return nullptr;
            }

            try 
            {
                return std::make_shared<usb_device_winusb>(info, devices_path);
            }
            catch (std::exception e)
            {
                LOG_WARNING("failed to create usb device: " << info.id << ", error: " << e.what());
            }
            return nullptr;
        }
    }
}
