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
#include "../types.h"
#include <mfapi.h>
#include <chrono>
#include <Windows.h>
#include <dbt.h>

namespace librealsense
{
    namespace platform
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

        class win_event_device_watcher : public device_watcher
        {
        public:
            win_event_device_watcher(const backend * backend)
            {
                _data._backend = backend;
                _data._stopped = false;
                _data._last = backend_device_group(backend->query_uvc_devices(), backend->query_usb_devices(), backend->query_hid_devices());
            }
            ~win_event_device_watcher() { stop(); }

            void start(device_changed_callback callback) override
            {
                std::lock_guard<std::mutex> lock(_m);
                if (!_data._stopped) throw wrong_api_call_sequence_exception("Cannot start a running device_watcher");
                _data._stopped = false;
                _data._callback = std::move(callback);
                _thread = std::thread([this]() { run(); });
            }

            void stop() override
            {
                std::lock_guard<std::mutex> lock(_m);
                if (!_data._stopped)
                {
                    _data._stopped = true;
                    if (_thread.joinable()) _thread.join();
                }
            }
        private:
            std::thread _thread;
            std::mutex _m;

            struct extra_data {
                const backend * _backend;
                backend_device_group _last;
                device_changed_callback _callback;

                bool _stopped;
                HWND hWnd;
                HDEVNOTIFY hdevnotifyHW, hdevnotifyUVC;
            } _data;

            void run()
            {
                WNDCLASS windowClass = {};
                LPCWSTR SzWndClass = TEXT("MINWINAPP");
                windowClass.lpfnWndProc = &on_win_event;
                windowClass.lpszClassName = SzWndClass;
                UnregisterClass(SzWndClass, nullptr);

                if (!RegisterClass(&windowClass))
                    LOG_WARNING("RegisterClass failed.");

                _data.hWnd = CreateWindow(SzWndClass, nullptr, 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, nullptr, &_data);
                if (!_data.hWnd)
                    throw winapi_error("CreateWindow failed");

                MSG msg;

                while (!_data._stopped)
                {
                    if (PeekMessage(&msg, _data.hWnd, 0, 0, PM_REMOVE))
                    {
                            TranslateMessage(&msg);
                            DispatchMessage(&msg);
                    }
                    else  // Yield CPU resources, as this is required for connect/disconnect events only
                        std::this_thread::sleep_for(std::chrono::milliseconds(50));
                }

                UnregisterDeviceNotification(_data.hdevnotifyHW);
                UnregisterDeviceNotification(_data.hdevnotifyUVC);
                DestroyWindow(_data.hWnd);
            }

            static LRESULT CALLBACK on_win_event(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
            {
                LRESULT lRet = 1;

                switch (message)
                {
                case WM_CREATE:
                    SetWindowLongPtr(hWnd, GWLP_USERDATA, LONG_PTR(reinterpret_cast<CREATESTRUCT*>(lParam)->lpCreateParams));
                    if (!DoRegisterDeviceInterfaceToHwnd(hWnd))
                case WM_QUIT:
                {
                    auto data = reinterpret_cast<extra_data*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
                    data->_stopped = true;
                    break;
                }
                case WM_DEVICECHANGE:
                {
                    //PDEV_BROADCAST_DEVICEINTERFACE b = (PDEV_BROADCAST_DEVICEINTERFACE)lParam;
                    // Output some messages to the window.
                    switch (wParam)
                    {
                    case DBT_DEVICEARRIVAL:
                    {
                        auto data = reinterpret_cast<extra_data*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
                        backend_device_group next(data->_backend->query_uvc_devices(), data->_backend->query_usb_devices(), data->_backend->query_hid_devices());
                        /*if (data->_last != next)*/ data->_callback(data->_last, next);
                        data->_last = next;
                    }
                        break;

                    case DBT_DEVICEREMOVECOMPLETE:
                    {
                        auto data = reinterpret_cast<extra_data*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
                        auto next = data->_last;
                        std::wstring temp = reinterpret_cast<DEV_BROADCAST_DEVICEINTERFACE*>(lParam)->dbcc_name;
                        std::string path;
                        path.reserve(temp.length());
                        for (wchar_t ch : temp) {
                            if (ch != L'{') path.push_back(std::tolower(((char*)&ch)[0]));
                            else break;
                        }

                        next.uvc_devices.erase(std::remove_if(next.uvc_devices.begin(), next.uvc_devices.end(), [&path](const uvc_device_info& info)
                        { return info.device_path.substr(0, info.device_path.find_first_of("{")) == path; }), next.uvc_devices.end());
                        //                            next.usb_devices.erase(std::remove_if(next.usb_devices.begin(), next.usb_devices.end(), [&path](const usb_device_info& info)
                        //                            { return info.device_path.substr(0, info.device_path.find_first_of("{")) == path; }), next.usb_devices.end());
                        next.usb_devices = data->_backend->query_usb_devices();
                        next.hid_devices.erase(std::remove_if(next.hid_devices.begin(), next.hid_devices.end(), [&path](const hid_device_info& info)
                        { return info.device_path.substr(0, info.device_path.find_first_of("{")) == path; }), next.hid_devices.end());

                        /*if (data->_last != next)*/ data->_callback(data->_last, next);
                        data->_last = next;
                    }
                        break;
                    }
                    break;
                }

                default:
                    // Send all other messages on to the default windows handler.
                    lRet = DefWindowProc(hWnd, message, wParam, lParam);
                    break;
                }

                return lRet;
            }

            static BOOL DoRegisterDeviceInterfaceToHwnd(HWND hWnd)
            {
                auto data = reinterpret_cast<extra_data*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

                //===========================register HWmonitor events==============================
                const GUID classGuid = { 0x175695cd, 0x30d9, 0x4f87, 0x8b, 0xe3, 0x5a, 0x82, 0x70, 0xf4, 0x9a, 0x31 };
                DEV_BROADCAST_DEVICEINTERFACE devBroadcastDeviceInterface;
                devBroadcastDeviceInterface.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
                devBroadcastDeviceInterface.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
                devBroadcastDeviceInterface.dbcc_classguid = classGuid;
                devBroadcastDeviceInterface.dbcc_reserved = 0;

                data->hdevnotifyHW = RegisterDeviceNotification(hWnd,
                    &devBroadcastDeviceInterface,
                    DEVICE_NOTIFY_WINDOW_HANDLE);
                if (data->hdevnotifyHW == NULL)
                {
                    LOG_WARNING("Register HW events Failed!\n");
                    return FALSE;
                }

                ////===========================register UVC events==============================
                DEV_BROADCAST_DEVICEINTERFACE di = { 0 };
                di.dbcc_size = sizeof(di);
                di.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
                di.dbcc_classguid = KSCATEGORY_CAPTURE;

                data->hdevnotifyUVC = RegisterDeviceNotification(hWnd,
                    &di,
                    DEVICE_NOTIFY_WINDOW_HANDLE);
                if (data->hdevnotifyUVC == nullptr)
                {
                    UnregisterDeviceNotification(data->hdevnotifyHW);
                    LOG_WARNING("Register UVC events Failed!\n");
                    return FALSE;
                }

                return TRUE;
            }
        };

        std::shared_ptr<device_watcher> wmf_backend::create_device_watcher() const
        {
            return std::make_shared<win_event_device_watcher>(this);
        }
    }
}

#endif
