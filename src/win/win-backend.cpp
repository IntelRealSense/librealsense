// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.
#ifdef RS2_USE_WMF_BACKEND

#if (_MSC_FULL_VER < 180031101)
    #error At least Visual Studio 2013 Update 4 is required to compile this backend
#endif

#include "win-backend.h"
#include "win-uvc.h"
#include "win-usb.h"
#include "win-hid.h"

#include <mfapi.h>
#include <chrono>
namespace rsimpl2
{
    namespace uvc
    {
        wmf_backend::wmf_backend()
        {
            CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
            MFStartup(MF_VERSION, MFSTARTUP_NOSOCKET);
        }

        wmf_backend::~wmf_backend()
        {
            try {
                MFShutdown();
                CoUninitialize();
            }
            catch(...)
            {
                // TODO: Write to log
            }
        }

        std::shared_ptr<uvc_device> wmf_backend::create_uvc_device(uvc_device_info info) const
        {
            return std::make_shared<retry_controls_work_around>(
                            std::make_shared<wmf_uvc_device>(info, shared_from_this()));
        }

        std::shared_ptr<backend> create_backend()
        {
            return std::make_shared<wmf_backend>();
        }

        std::vector<uvc_device_info> wmf_backend::query_uvc_devices() const
        {
            std::vector<uvc_device_info> devices;

            auto action = [&devices](const uvc_device_info& info, IMFActivate*)
            {
                devices.push_back(info);
            };

            wmf_uvc_device::foreach_uvc_device(action);

            return devices;
        }

        std::shared_ptr<usb_device> wmf_backend::create_usb_device(usb_device_info info) const
        {
            return std::make_shared<winusb_bulk_transfer>(info);
        }

        std::vector<usb_device_info> wmf_backend::query_usb_devices() const
        {
            const std::vector<std::string> usb_interfaces = {
                "{175695CD-30D9-4F87-8BE3-5A8270F49A31}",
                "{08090549-CE78-41DC-A0FB-1BD66694BB0C}"
            };

            std::vector<usb_device_info> result;
            for (auto&& interface_id : usb_interfaces)
            {
                for (auto&& id : usb_enumerate::query_by_interface(interface_id, "", ""))
                {
                    std::string path(id.begin(), id.end());
                    uint16_t vid, pid, mi; std::string unique_id;
                    if (!parse_usb_path(vid, pid, mi, unique_id, path)) continue;

                    usb_device_info info{ path, vid, pid, mi, unique_id};
                    /*info.id = path;
                    info.vid = vid;
                    info.pid = pid;
                    info.unique_id = unique_id;
                    info.mi = mi;*/

                    result.push_back(info);
                }
            }

            return result;
        }

        std::shared_ptr<hid_device> wmf_backend::create_hid_device(hid_device_info info) const
        {
            std::shared_ptr<hid_device> result = nullptr;

            auto action = [&result, &info](const hid_device_info& i, CComPtr<ISensor> ptr)
            {
                if (info.device_path == i.device_path)
                {
                    result = std::make_shared<wmf_hid_device>(ptr);
                }
            };

            wmf_hid_device::foreach_hid_device(action);

            if (result.get()) return result;
            throw std::runtime_error("Device no longer found!");
        }

        std::vector<hid_device_info> wmf_backend::query_hid_devices() const
        {
            std::vector<hid_device_info> devices;

            auto action = [&devices](const hid_device_info& info, CComPtr<ISensor>)
            {
                devices.push_back(info);
            };

            wmf_hid_device::foreach_hid_device(action);

            return devices;
        }

        std::shared_ptr<time_service> wmf_backend::create_time_service() const
        {
            return std::make_shared<os_time_service>();
        }

    }
}

#endif
