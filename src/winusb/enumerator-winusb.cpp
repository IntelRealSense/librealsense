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

const std::vector<std::string> guids = {
    "{175695CD-30D9-4F87-8BE3-5A8270F49A31}", //Ivcam
    "{08090549-CE78-41DC-A0FB-1BD66694BB0C}", //D4xx
    "{a5dcbf10-6530-11d2-901f-00c04fb951ed}"  //DFU
};

namespace librealsense
{
    namespace platform
    {
        std::vector<std::wstring> query_by_interface(GUID guid, std::wstring vid)
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
                    std::transform(vid.begin(), vid.end(), vid.begin(), std::toupper);
                    if (str.find(vid) != std::string::npos)
                        rv.push_back(str);
                    offset += str.size() + 1;
                }
            } while (cr == CR_BUFFER_SMALL);

            return rv;
        }

        std::vector<std::wstring> query_by_interface(const std::string& guidStr, std::wstring vid)
        {
            GUID guid;
            std::wstring guidWStr(guidStr.begin(), guidStr.end());
            CHECK_HR(CLSIDFromString(guidWStr.c_str(), static_cast<LPCLSID>(&guid)));
            return query_by_interface(guid, vid);
        }

        std::string get_camera_id(const wchar_t* deviceID)
        {
            std::wsmatch matches;
            std::wstring deviceIDstr(deviceID);

            std::wregex recoveryReg(L"#111111111111#", std::regex_constants::icase);
            if (std::regex_search(deviceIDstr, matches, recoveryReg))
                return "recovery";

            std::wregex camIdReg(L"\\b(mi_)(.*#\\w&)(.*)(&.*&)", std::regex_constants::icase);
            if (!std::regex_search(deviceIDstr, matches, camIdReg) || matches.size() < 5)
                throw std::runtime_error("regex_search failed!");

            std::wstring wdevID = matches[3];
            return std::string(wdevID.begin(), wdevID.end());
        }

        bool usb_enumerator::is_device_connected(const std::shared_ptr<usb_device>& device)
        {
            if (device == nullptr)
                return false;
            for (auto&& guid : guids)
            {
                for (auto&& id : query_by_interface(guid, L"8086"))
                {
                    auto expected_id = get_camera_id(id.c_str());
                    auto curr_id = device->get_info().id;
                    if (curr_id == expected_id)
                        return true;
                }
            }
            return false;
        }

        std::vector<std::shared_ptr<usb_device>> usb_enumerator::query_devices()
        {
            std::map<std::string, std::vector<std::wstring>> devices_path;

            for (auto&& guid : guids)
            {
                for (auto&& id : query_by_interface(guid, L"8086"))
                {
                    try {
                        auto s = get_camera_id(id.c_str());
                        devices_path[s].push_back(id);
                    }
                    catch (std::exception e) {
                        std::string path(id.begin(), id.end());
                        LOG_WARNING("failed to create USB device, device: " << path.c_str() << ", error: " << e.what());
                    }
                }
            }

            std::vector<std::shared_ptr<usb_device>> rv;
            for (auto&& id : devices_path)
            {
                try {
                    rv.push_back(std::make_shared<usb_device_winusb>(id.second, id.first == "recovery"));
                }
                catch (...)
                {
                    LOG_WARNING("failed to create usb device: %s" << id.first.c_str());
                }
            }
            return rv;
        }
    }
}
