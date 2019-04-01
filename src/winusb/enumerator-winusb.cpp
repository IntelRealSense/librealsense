// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#ifdef WIN32

#if (_MSC_FULL_VER < 180031101)
#error At least Visual Studio 2013 Update 4 is required to compile this backend
#endif

#include "usb/usb-enumerator.h"
#include "device-winusb.h"
#include "interface-winusb.h"
#include "win/win-helpers.h"
#include "types.h"
#include "ds5/ds5-private.h"
#include "l500/l500-private.h"
#include "ivcam/sr300.h"

#include <atlstr.h>
#include <Windows.h>
#include <Sddl.h>
#include <string>
#include <regex>
#include <sstream>
#include <mutex>

#include <SetupAPI.h>

#pragma comment(lib, "winusb.lib")

namespace librealsense
{
    namespace platform
    {
        std::string to_hex(uint16_t val)
        {
            std::stringstream stream;
            stream << std::setfill('0') << std::setw(4) << std::hex << val;
            return stream.str();
        }

        std::vector<uint16_t> get_pids()
        {
            std::vector<uint16_t> rv;
            rv.insert(rv.end(), ds::rs400_sku_pid.begin(), ds::rs400_sku_pid.end());
            rv.push_back(SR300_PID);
            rv.push_back(SR300v2_PID);
            rv.push_back(L500_PID);
            return rv;
        }

        std::vector<uint16_t> get_recovery_pids()
        {
            std::vector<uint16_t> rv;
            rv.insert(rv.end(), ds::rs400_sku_recovery_pid.begin(), ds::rs400_sku_recovery_pid.end());
            return rv;
        }

        bool get_device_data(_GUID guid, int i, const HDEVINFO& deviceInfo, std::wstring& lpDevicePath)
        {
            SP_DEVICE_INTERFACE_DATA interfaceData;
            unsigned long length;
            unsigned long requiredLength = 0;

            std::shared_ptr<SP_DEVICE_INTERFACE_DETAIL_DATA> detailData;

            // Obtain information on your device interface
            //Enumerate all the device interfaces in the device information set.
            memset(&interfaceData, 0, sizeof(interfaceData));
            interfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

            if (!SetupDiEnumDeviceInterfaces(deviceInfo, nullptr, &guid, i, &interfaceData))
            {
                return false;
            }

            // Get detailed data for the device interface
            //Interface data is returned in SP_DEVICE_INTERFACE_DETAIL_DATA
            //which we need to allocate, so we have to call this function twice.
            //First to get the size so that we know how much to allocate
            //Second, the actual call with the allocated buffer
            if (!SetupDiGetDeviceInterfaceDetail(deviceInfo, &interfaceData, nullptr, 0, &requiredLength, nullptr))
            {
                if ((ERROR_INSUFFICIENT_BUFFER == GetLastError()) && (requiredLength > 0))
                {
                    //we got the size, allocate buffer
                    auto * detailData1 = static_cast<PSP_DEVICE_INTERFACE_DETAIL_DATA>(LocalAlloc(LMEM_FIXED, requiredLength));
                    if (!detailData1)
                        return false;

                    // TODO: Make proper PSP_DEVICE_INTERFACE_DETAIL_DATA wrapper
                    detailData = std::shared_ptr<SP_DEVICE_INTERFACE_DETAIL_DATA>(detailData1,
                        [=](SP_DEVICE_INTERFACE_DETAIL_DATA*)
                    {
                        if (detailData1 != nullptr)
                        {
                            LocalFree(detailData1);
                        }
                    });
                }
                else
                {
                    return false;
                }
            }

            if (nullptr == detailData)
                throw winapi_error("SP_DEVICE_INTERFACE_DETAIL_DATA object is null!");

            detailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
            length = requiredLength;

            if (!SetupDiGetDeviceInterfaceDetail(deviceInfo, &interfaceData, detailData.get(), length, &requiredLength, nullptr))
            {
                return false;
            }

            // Get the device path
            lpDevicePath = detailData->DevicePath;

            return true;
        }

        std::vector<std::wstring> query_by_interface(const std::string& guidStr, const std::string& vid, const std::string& pid)
        {
            GUID guid;
            std::wstring guidWStr(guidStr.begin(), guidStr.end());
            CHECK_HR(CLSIDFromString(guidWStr.c_str(), static_cast<LPCLSID>(&guid)));

            HDEVINFO deviceInfo;
            std::vector<std::wstring> retVec;

            deviceInfo = SetupDiGetClassDevs(&guid, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
            if (deviceInfo != INVALID_HANDLE_VALUE)
            {
                for (int i = 0; i < 256; i++)
                {
                    std::wstring lpDevicePath;

                    if (!get_device_data(guid, i, deviceInfo, lpDevicePath))
                        continue;

                    std::wstring wVid(vid.begin(), vid.end());
                    std::wstring wPid(pid.begin(), pid.end());

                    auto vidPos = lpDevicePath.find(std::wstring(L"vid_") + std::wstring(vid.begin(), vid.end()));
                    if (vidPos == std::wstring::npos)
                    {
                        continue;
                    }

                    auto pidPos = lpDevicePath.find(std::wstring(L"pid_") + std::wstring(pid.begin(), pid.end()));
                    if (pidPos == std::wstring::npos)
                    {
                        continue;
                    }

                    retVec.push_back(lpDevicePath);
                }
            }
            else
            {
                throw winapi_error("SetupDiGetClassDevs failed.");
            }

            if (!SetupDiDestroyDeviceInfoList(deviceInfo))
            {
                throw winapi_error("Failed to clean-up SetupDiDestroyDeviceInfoList.");
            }

            return retVec;
        }

        std::string get_camera_id(const wchar_t* deviceID)
        {
            std::wsmatch matches;
            std::wstring deviceIDstr(deviceID);

            std::wregex camIdReg(L"\\b(mi_)(.*#\\w&)(.*)(&.*&)", std::regex_constants::icase);
            if (!std::regex_search(deviceIDstr, matches, camIdReg) || matches.size() < 5)
                throw std::runtime_error("regex_search failed!");

            std::wstring wdevID = matches[3];
            return std::string(wdevID.begin(), wdevID.end());
        }

        bool usb_enumerator::is_device_connected(const std::shared_ptr<usb_device> device)
        {
            const std::vector<std::string> usb_interfaces = {
                "{175695CD-30D9-4F87-8BE3-5A8270F49A31}",
                "{08090549-CE78-41DC-A0FB-1BD66694BB0C}",
                "{a5dcbf10-6530-11d2-901f-00c04fb951ed}"
            };

            for (auto&& interface_id : usb_interfaces)
            {
                for (auto&& id : query_by_interface(interface_id, "", ""))
                {
                    std::string path(id.begin(), id.end());
                    if (device->get_info().id == path)//TODO:MK select one  id? path? unique id?
                        return true;
                }
            }
            return false;
        }

        std::vector<std::shared_ptr<usb_device>> usb_enumerator::query_devices()
        {
            std::map<std::string, std::vector<std::wstring>> devices_path;

            const std::vector<std::string> fw_update_devices = {
                "{a5dcbf10-6530-11d2-901f-00c04fb951ed}"
            };

            const std::vector<std::string> usb_interfaces = {
                //"{65e8773d-8f56-11d0-a3b9-00a0c9223196}", //00, 03
                //"{6994ad05-93ef-11d0-a3cc-00a0c9223196}", //00, 03
                //"{e5323777-f976-4f5b-9b55-b94699c46e44}", //00, 03
                //"{dee824ef-729b-4a0e-9c14-b7117d33a817}", //05
                "{08090549-ce78-41dc-a0fb-1bd66694bb0c}", //05
                "{175695CD-30D9-4F87-8BE3-5A8270F49A31}", //win7?
                //"{cc7bfb41-f175-11d1-a392-00e0291f3959}", //win7?
                //"{de91e8e5-7e65-5114-bf52-94b56d4f3877}", //win7?
            };

            auto pids = get_pids();
            for (auto&& pid : pids)
            {
                for (auto&& interface_id : usb_interfaces)
                {
                    for (auto&& id : query_by_interface(interface_id, "8086", to_hex(pid)))
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
            }

            auto recovery_pids = get_recovery_pids();
            for (auto&& pid : recovery_pids)
            {
                for (auto&& interface_id : fw_update_devices)
                {
                    for (auto&& id : query_by_interface(interface_id, "8086", to_hex(pid)))
                    {
                        devices_path["recovery"].push_back(id);
                    }
                }
            }

            std::vector<std::shared_ptr<usb_device>> rv;
            for (auto&& id : devices_path)
            {
                rv.push_back(std::make_shared<usb_device_winusb>(id.second, id.first == "recovery"));
            }
            return rv;
        }
    }
}
#endif