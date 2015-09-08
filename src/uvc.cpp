#include "uvc.h"

#ifndef WIN32
#include "libuvc/libuvc.h"
#include "libuvc/libuvc_internal.h" // For LibUSB punchthrough
#include <iostream>

namespace rsimpl
{
    namespace uvc
    {
        static void check(const char * call, uvc_error_t status)
        {
            if (status < 0) throw std::runtime_error(to_string() << call << "(...) returned " << uvc_strerror(status));
        }

        struct context::_impl
        {
            uvc_context_t * ctx;

            _impl() : ctx() { check("uvc_init", uvc_init(&ctx, nullptr)); }
            ~_impl() { if(ctx) uvc_exit(ctx); }
        };

        struct device::_impl
        {
            std::shared_ptr<context::_impl> parent;
            uvc_device_t * device;
            uvc_device_descriptor_t * desc;
            std::vector<uvc_device_handle_t *> handles;
            std::vector<int> claimed_interfaces;

            _impl() : device(), desc() {}
            _impl(std::shared_ptr<context::_impl> parent, uvc_device_t * device) : _impl()
            {
                this->parent = parent;
                this->device = device;
                uvc_ref_device(device);
                check("uvc_get_device_descriptor", uvc_get_device_descriptor(device, &desc));
            }
            ~_impl()
            {
                for(auto interface_number : claimed_interfaces)
                {
                    int status = libusb_release_interface(get_handle(0)->usb_devh, interface_number);
                    if(status < 0) std::cerr << "Warning: libusb_release_interface(...) returned " << libusb_error_name(status) << std::endl;
                }

                for(auto * h : handles) if(h) uvc_close(h);
                if(desc) uvc_free_device_descriptor(desc);
                if(device) uvc_unref_device(device);
            }

            uvc_device_handle_t * get_handle(int subdevice_index)
            {
                if(subdevice_index >= handles.size()) handles.resize(subdevice_index+1);
                if(!handles[subdevice_index]) check("uvc_open2", uvc_open2(device, &handles[subdevice_index], subdevice_index));
                return handles[subdevice_index];
            }
        };

        struct device_handle::_impl
        {
            std::shared_ptr<device::_impl> parent;
            int subdevice_index;
            uvc_stream_ctrl_t ctrl;
            std::function<void(const void * frame)> callback;

            _impl(std::shared_ptr<device::_impl> parent, int subdevice_index) : parent(parent), subdevice_index(subdevice_index) { get_handle(); }
            ~_impl() { uvc_stop_streaming(get_handle()); } // UVC callback has a naked ptr to this struct, must make sure it's done before we destruct

            uvc_device_handle_t * get_handle() { return parent->get_handle(subdevice_index); }
        };

        ///////////////////
        // device_handle //
        ///////////////////

        void device_handle::set_mode(int width, int height, frame_format cf, int fps)
        {
            check("get_stream_ctrl_format_size", uvc_get_stream_ctrl_format_size(impl->get_handle(), &impl->ctrl, (uvc_frame_format)cf, width, height, fps));
        }

        void device_handle::start_streaming(std::function<void(const void * frame)> callback)
        {
            #if defined (ENABLE_DEBUG_SPAM)
            uvc_print_stream_ctrl(&impl->ctrl, stdout);
            #endif

            impl->callback = callback;
            check("uvc_start_streaming", uvc_start_streaming(impl->get_handle(), &impl->ctrl, [](uvc_frame * frame, void * user)
            {
                reinterpret_cast<device_handle::_impl *>(user)->callback(frame->data);
            }, impl.get(), 0));
        }

        void device_handle::stop_streaming()
        {
            uvc_stop_streaming(impl->get_handle());
        }

        ////////////
        // device //
        ////////////

        int device::get_vendor_id() const { return impl->desc->idVendor; }
        int device::get_product_id() const { return impl->desc->idProduct; }

        void device::get_control(uint8_t unit, uint8_t ctrl, void * data, int len) const
        {
            int status = uvc_get_ctrl(impl->get_handle(0), unit, ctrl, data, len, UVC_GET_CUR);
            if(status < 0) throw std::runtime_error(to_string() << "uvc_get_ctrl(...) returned " << libusb_error_name(status));
        }

        void device::set_control(uint8_t unit, uint8_t ctrl, void * data, int len)
        {
            int status = uvc_set_ctrl(impl->get_handle(0), unit, ctrl, data, len);
            if(status < 0) throw std::runtime_error(to_string() << "uvc_set_ctrl(...) returned " << libusb_error_name(status));
        }

        void device::claim_interface(int interface_number)
        {
            int status = libusb_claim_interface(impl->get_handle(0)->usb_devh, interface_number);
            if(status < 0) throw std::runtime_error(to_string() << "libusb_claim_interface(...) returned " << libusb_error_name(status));
            impl->claimed_interfaces.push_back(interface_number);
        }

        void device::bulk_transfer(unsigned char endpoint, unsigned char *data, int length, int *actual_length, unsigned int timeout)
        {
            int status = libusb_bulk_transfer(impl->get_handle(0)->usb_devh, endpoint, data, length, actual_length, timeout);
            if(status < 0) throw std::runtime_error(to_string() << "libusb_bulk_transfer(...) returned " << libusb_error_name(status));
        }

        device_handle device::claim_subdevice(int subdevice_index)
        {
            return {std::make_shared<device_handle::_impl>(impl, subdevice_index)};
        }

        /////////////
        // context //
        /////////////

        context context::create()
        {
            return {std::make_shared<context::_impl>()};
        }

        std::vector<device> context::query_devices()
        {
            uvc_device_t ** list;
            check("uvc_get_device_list", uvc_get_device_list(impl->ctx, &list));
            std::vector<device> devices;
            for(auto it = list; *it; ++it)
            {
                devices.push_back({std::make_shared<device::_impl>(impl, *it)});
            }
            uvc_free_device_list(list, 1);
            return devices;
        }
    }
}

#else

#include <Shlwapi.h>        // For QISearch, etc.
#include <mfapi.h>          // For MFStartup, etc.
#include <mfidl.h>          // For MF_DEVSOURCE_*, etc.
#include <mfreadwrite.h>    // MFCreateSourceReaderFromMediaSource
#include <mferror.h>

#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")

#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "winusb.lib")

#include <uuids.h>
#include <vidcap.h>
#include <ksmedia.h>
#include <ksproxy.h>

#include <SetupAPI.h>
#include <WinUsb.h>
#include <atlstr.h>

#include <thread>
#include <chrono>
#include <algorithm>
#include <iostream>
#include <regex>

#include <strsafe.h>

#define XUNODEID 1

DEFINE_GUID(IVCAM_WIN_USB_DEVICE_GUID_2, 0x8EC3DC67, 0xE0E0, 0x4F04, 0x85, 0x19, 0xC2, 0x77, 0x52, 0x5B, 0x4B, 0xEA);
DEFINE_GUID(IVCAM_WIN_USB_DEVICE_GUID, 0x175695CD, 0x30D9, 0x4F87, 0x8B, 0xE3, 0x5A, 0x82, 0x70, 0xF4, 0x9A, 0x31);

namespace rsimpl
{
    namespace uvc
    {

		        static std::string win_to_utf(const WCHAR * s)
        {
            int len = WideCharToMultiByte(CP_UTF8, 0, s, -1, nullptr, 0, NULL, NULL);
            if(len == 0) throw std::runtime_error(to_string() << "WideCharToMultiByte(...) returned 0 and GetLastError() is " << GetLastError());
            std::string buffer(len-1, ' ');
            len = WideCharToMultiByte(CP_UTF8, 0, s, -1, &buffer[0], (int)buffer.size()+1, NULL, NULL);
            if(len == 0) throw std::runtime_error(to_string() << "WideCharToMultiByte(...) returned 0 and GetLastError() is " << GetLastError());
            return buffer;
        }

        static void check(const char * call, HRESULT hr)
        {
            if(FAILED(hr)) throw std::runtime_error(to_string() << call << "(...) returned 0x" << std::hex << (uint32_t)hr);
        }

        template<class T> class com_ptr
        {
            T * p;

            void ref(T * new_p)
            {
                if(p == new_p) return;
                unref();
                p = new_p;
                if(p) p->AddRef();
            }

            void unref()
            {
                if(p)
                {
                    p->Release();
                    p = nullptr;
                }
            }
        public:
            com_ptr() : p() {}
            com_ptr(T * p) : com_ptr() { ref(p); }
            com_ptr(const com_ptr & r) : com_ptr(r.p) {}
            ~com_ptr() { unref(); }

            operator T * () const { return p; }
            T & operator * () const { return *p; }
            T * operator -> () const { return p; }

            T ** operator & () { unref(); return &p; }
            com_ptr & operator = (const com_ptr & r) { ref(r.p); return *this; }            
        };

		namespace winusb
		{
			enum class WinUsbEnumerationAction
			{
				BreakWithSuccess,
				BreakWithError,
				BreakWithRetryError,
				Continue
			};

			enum WinUsbStatus
			{
				Success,
				Failure,
				TempraryFailureTryToRecallFunction
			};

			#define WIN_USB_CALL(x)										\
			{	auto res = x;											\
				if (res != Success)										\
				{														\
					std::cerr << "usb_usb_call(x) error \n";			\
					return res;											\
				}														\
			}

			enum class IvcamUsbDevice
			{
				None			= -1,
				RGB				=  0,
				Depth			=  2,
				HardwareMonitor	=  4
			};

			class OnScopeExit
			{
				std::function<void()> _releaseFunc;
			public:
				OnScopeExit(std::function<void()> releaseFunc)
					:_releaseFunc(releaseFunc) { }
				~OnScopeExit() { _releaseFunc(); }
				void Cancel() { _releaseFunc = std::function<void()>([](){});}
			};

			using USB_GUID_T = std::vector<const _GUID *>;
			using WinUsbCustomFunc = std::function<WinUsbEnumerationAction(int, HDEVINFO, WCHAR *)>;

			class WinUsbUtils
			{
				#define IVCAM_VID				L"8086"
				#define IVCAM_PID				L"0A66"
				#define IVCAM_DEVICE_NAME		L"IVCAM"

				#define IVCAM_USB2_PID			L"0AA1"
				#define SKYCAM_OPER_PID			L"0AA2"
				#define SKYCAM_UVC_PID			L"0AA3"
				#define REALTEK_OPER_PID		L"0AA4"
				#define REALTEK_DBG_PID			L"0AA5"

				static WinUsbStatus for_each_device(const _GUID * guid, const WinUsbCustomFunc & action)
				{
					auto result = WinUsbEnumerationAction::Continue;
					HDEVINFO deviceInfo;

					deviceInfo = SetupDiGetClassDevs(guid, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
					if (deviceInfo != INVALID_HANDLE_VALUE)
					{
						for (int i = 0; i < 256; i++)
						{
							std::shared_ptr<WCHAR> lpDevicePath;
							if (!get_device_data(i, guid, deviceInfo, lpDevicePath))
								continue;

							result = action(i, deviceInfo, lpDevicePath.get());
							if (result != WinUsbEnumerationAction::Continue)
								break;
						}
					}
					else
					{
						throw std::runtime_error("SetupDiGetClassDevs");
						return Failure;
					}

					if (!SetupDiDestroyDeviceInfoList(deviceInfo))
						std::cerr << "Failed to clean-up SetupDiDestroyDeviceInfoList!" << std::endl;

					switch (result)
					{
					case WinUsbEnumerationAction::BreakWithRetryError:
						return TempraryFailureTryToRecallFunction;
					case WinUsbEnumerationAction::BreakWithError:
						return Failure;
					case WinUsbEnumerationAction::BreakWithSuccess:
					case WinUsbEnumerationAction::Continue:
						return Success;
					}

					return Failure;
				}

				static BOOL get_device_data(int i, const _GUID* guid, const HDEVINFO & deviceInfo, std::shared_ptr<WCHAR> & lpDevicePath)
				{
					SP_DEVICE_INTERFACE_DATA interfaceData;
					unsigned long length;
					unsigned long requiredLength = 0;

					std::shared_ptr<SP_DEVICE_INTERFACE_DETAIL_DATA> detailData;

					// Enumerate all the device interfaces in the device information set.
					memset(&interfaceData, 0, sizeof(interfaceData));
					interfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

					if (!SetupDiEnumDeviceInterfaces(deviceInfo, nullptr, guid, i, &interfaceData))
						return FALSE;

					if (!SetupDiGetDeviceInterfaceDetail(deviceInfo, &interfaceData, nullptr, 0, &requiredLength, nullptr))
					{
						if ((ERROR_INSUFFICIENT_BUFFER == GetLastError()) && (requiredLength > 0))
						{
							//we got the size, allocate buffer
							auto * detailData1 = static_cast<PSP_DEVICE_INTERFACE_DETAIL_DATA>(LocalAlloc(LMEM_FIXED, requiredLength));
							if (!detailData1)
								return Failure;

							detailData = std::shared_ptr<SP_DEVICE_INTERFACE_DETAIL_DATA>(detailData1,[=](SP_DEVICE_INTERFACE_DETAIL_DATA *)
							{
								if (detailData1 != nullptr) LocalFree(detailData1);
							});
						}
						else
						{
							std::cerr << "SetupDiGetDeviceInterfaceDetail failed" << std::endl;
							return FALSE;
						}
					}

					detailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
					length = requiredLength;

					if (!SetupDiGetDeviceInterfaceDetail(deviceInfo, &interfaceData, detailData.get(), length, &requiredLength, nullptr))
					{
						std::cerr << "SetupDiGetDeviceInterfaceDetail failed" << std::endl;
						return FALSE;
					}

					// Get the device path 
					auto nLength = wcslen(detailData->DevicePath) + 1;
					auto lpDevicePath1 = static_cast<WCHAR*>(LocalAlloc(LPTR, nLength * sizeof(WCHAR)));
					if (lpDevicePath1 == nullptr)
					{
						std::cerr << "failed to get path" << std::endl;
						return FALSE;
					}

					auto hr = StringCchCopy(lpDevicePath1, nLength, detailData->DevicePath);
					lpDevicePath1[nLength-1] = 0;
					if (FAILED(hr))
					{
						std::cerr << "failed StringCchCopy()" << std::endl;
						return FALSE;
					}

					lpDevicePath = std::shared_ptr<WCHAR>(lpDevicePath1, [=](WCHAR*)
					{
						if (lpDevicePath1 != nullptr) LocalFree(lpDevicePath1);
					});

					return TRUE;
				}
				static bool contains_pid_substring(const wchar_t* str, const std::vector<LPWSTR>& ivcam_pid)
				{
					CString StrCopy(str);
					for (auto pid : ivcam_pid)
					{
						CString pidStr(pid);
						pidStr = CString(L"&pid_") + pidStr + CString(L"&");
						int num = StrCopy.MakeLower().Find(pidStr.MakeLower());
						if ((num >= 0) && (num < StrCopy.GetLength()))
							return true;
					}
					return false;
				}

				static IvcamUsbDevice get_camera_usb_type(const wchar_t * deviceID)
				{	
					std::wsmatch matches;
					std::wstring deviceIDstr(deviceID);
					std::wregex camTypeReg(L"\\b(mi_|MI_)(\\d\\d)");
	
					if (!std::regex_search(deviceIDstr, matches, camTypeReg) || matches.size() < 3)
						return IvcamUsbDevice::None;

					std::wstring cam = matches[2];
					auto camType = static_cast<IvcamUsbDevice>(_wtoi(cam.c_str()));

					if (camType == IvcamUsbDevice::RGB || camType == IvcamUsbDevice::Depth || camType == IvcamUsbDevice::HardwareMonitor)
						return camType;

					return IvcamUsbDevice::None;
				}

				bool static get_camera_usb_id(IN const wchar_t* deviceID, OUT std::string *devID)
				{
					std::wsmatch matches;
					std::wstring deviceIDstr(deviceID);

					std::wregex camIdReg(L"\\b(mi_)(.*#\\w&)(.*)(&.*&)", std::regex_constants::icase);
					if (!std::regex_search(deviceIDstr, matches, camIdReg) || matches.size() < 5)
						return false;

					std::wstring wdevID = matches[3];
					*devID = win_to_utf(wdevID.c_str());
					return true;
				}

			public:
				static WinUsbStatus for_each_device(const USB_GUID_T & guids, const WinUsbCustomFunc & action)
				{
					for (auto pGuid : guids)
						WIN_USB_CALL(for_each_device(pGuid, action));
					return Success;
				}

				static bool is_ivcam_pid(const wchar_t * deviceID)
				{
					return (contains_pid_substring(deviceID, {IVCAM_PID, IVCAM_USB2_PID}) 
						||  contains_pid_substring(deviceID, {SKYCAM_OPER_PID, SKYCAM_UVC_PID, REALTEK_OPER_PID, REALTEK_DBG_PID}));
				}

				bool static parse_device_id(IN const wchar_t* deviceID, OUT  std::string* devID, OUT IvcamUsbDevice * camType)
				{
					if (deviceID == nullptr)
						throw std::runtime_error("device id is null");

					auto resultingCamType = get_camera_usb_type(deviceID);
					if (resultingCamType == IvcamUsbDevice::None)
						return false;

					std::string resultingDevId;
					if (!get_camera_usb_id(deviceID, &resultingDevId))
					{
						return false;
					}

					*devID = resultingDevId;
					*camType = resultingCamType;
					return true;
				}
			};
		}

        struct context::_impl
        {
            IMFAttributes * pAttributes = nullptr;
            IMFActivate ** ppDevices = nullptr;

            _impl()
            {
                CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                MFStartup(MF_VERSION, MFSTARTUP_NOSOCKET);
            }
            ~_impl()
            {   
                MFShutdown();
                CoUninitialize();
            }
        };

        struct device::_impl
        {
            const std::shared_ptr<context::_impl> parent;
            const int vid, pid;
            const std::string unique_id;

            std::vector<com_ptr<IMFActivate>> subdevices;
            std::vector<com_ptr<IMFMediaSource>> sources;
            com_ptr<IKsControl> ks_control;

			HANDLE winUsbHandle = nullptr;
			WINUSB_INTERFACE_HANDLE usbHandle = nullptr;
			UCHAR  dataInPipe;		
			UCHAR  dataOutPipe;
			char ivcamCameraId[256];
			bool winUsbOpen = false;

            _impl(std::shared_ptr<context::_impl> parent, int vid, int pid, std::string unique_id) : parent(move(parent)), vid(vid), pid(pid), unique_id(move(unique_id)) {}

            com_ptr<IMFMediaSource> get_media_source(int subdevice_index)
            {
                if(!sources[subdevice_index]) check("IMFActivate::ActivateObject", subdevices[subdevice_index]->ActivateObject(__uuidof(IMFMediaSource), (void **)&sources[subdevice_index]));
                return sources[subdevice_index];
            }

			bool iterate_win_usb_devices(const winusb::WinUsbCustomFunc & action)
			{
				winusb::USB_GUID_T guidList {&IVCAM_WIN_USB_DEVICE_GUID, &IVCAM_WIN_USB_DEVICE_GUID_2};
				return winusb::WinUsbUtils::for_each_device(guidList, action);
			}

			bool query_device_endpoints()
			{
				if (usbHandle == INVALID_HANDLE_VALUE || usbHandle == nullptr)
					throw std::runtime_error("invalid usbHandle");

				USB_INTERFACE_DESCRIPTOR desc;
				ZeroMemory(&desc, sizeof(USB_INTERFACE_DESCRIPTOR));

				WINUSB_PIPE_INFORMATION  Pipe;
				ZeroMemory(&Pipe, sizeof(WINUSB_PIPE_INFORMATION));

				auto result = WinUsb_QueryInterfaceSettings(usbHandle, 0, &desc);

				if (result)
				{
					for (auto index = (UCHAR)0; index < desc.bNumEndpoints; index++)
					{
						result = WinUsb_QueryPipe(usbHandle, 0, index, &Pipe);
						if (result)
						{
							if (Pipe.PipeType == UsbdPipeTypeBulk)
							{
								if (USB_ENDPOINT_DIRECTION_IN(Pipe.PipeId))
								{
									dataInPipe = Pipe.PipeId;
								}
								if (USB_ENDPOINT_DIRECTION_OUT(Pipe.PipeId))
								{
									dataOutPipe = Pipe.PipeId;
								}
							}
						}
						else continue;
					}
				}
				return result;
			}

			void init_usb_pipe()
			{
				if (winUsbHandle == nullptr)
					throw std::runtime_error("winUsbHandle has not been opened");

				auto result = WinUsb_Initialize(IN winUsbHandle, OUT &usbHandle);
				if (!result)
				{
					std::cerr << "Last Error: " << GetLastError() << std::endl;
					close_win_usb();
					throw std::runtime_error("could not initialize winusb");
				}

				result = query_device_endpoints();
				if (!result)
				{
					std::cerr << "Last Error: " << GetLastError() << std::endl;
					close_win_usb();
					throw std::runtime_error("could not query endpoints");
				}
			}

			void open_win_usb()
			{
				auto result = iterate_win_usb_devices([&](int i, HDEVINFO deviceInfo, LPTSTR lpDevicePath)
				{
					std::string devid;
					winusb::IvcamUsbDevice cameraNum;

					if (!winusb::WinUsbUtils::is_ivcam_pid(lpDevicePath))
						return winusb::WinUsbEnumerationAction::Continue;

					if (!winusb::WinUsbUtils::parse_device_id(lpDevicePath, &devid, &cameraNum))
						return winusb::WinUsbEnumerationAction::Continue;

					if (strcmp(ivcamCameraId, devid.c_str()))
						return winusb::WinUsbEnumerationAction::Continue;

					auto fileHandle = CreateFile(lpDevicePath, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_WRITE | FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, nullptr);

					// Argh, refactor this crap
					winusb::OnScopeExit releaseFileHandle([&] { close_win_usb(); });

					if (fileHandle != INVALID_HANDLE_VALUE)
						winUsbHandle = fileHandle;
					else
						throw std::runtime_error("CreateFile for path returned invalid_handle"); // lpDevicePath

					init_usb_pipe();

					releaseFileHandle.Cancel();

					return winusb::WinUsbEnumerationAction::Continue;
				});

				if (result)
				{
					winUsbOpen = true;
				}
			}

			void close_win_usb()
			{
				if (usbHandle != nullptr && usbHandle != INVALID_HANDLE_VALUE)
				{
					WinUsb_Free(usbHandle);
					usbHandle = nullptr;
				}
			}

			bool wait_for_async_transfer(OVERLAPPED & hOvl, ULONG & lengthTransferred, DWORD TimeOut, UCHAR pipeId)
			{
				if (!winUsbOpen) throw std::runtime_error("winusb has not been initialized");

				bool result = false;

				auto rc = GetOverlappedResult(winUsbHandle, &hOvl, &lengthTransferred, FALSE);
				if (rc == FALSE)
				{
					auto lastResult = GetLastError();
					if (lastResult == ERROR_IO_PENDING || lastResult == ERROR_IO_INCOMPLETE)
					{
						auto hResult = WaitForSingleObject(hOvl.hEvent, TimeOut);
						if (hResult == WAIT_TIMEOUT)
						{
							lengthTransferred = 0;
							WinUsb_ResetPipe(usbHandle, dataInPipe);
							WinUsb_ResetPipe(usbHandle, dataOutPipe);		
						}
						else
						{
							if (GetOverlappedResult(winUsbHandle, &hOvl, &lengthTransferred, FALSE) == 1)
								result = true;
							else
								result = false;
						}
					}
					else
					{
						lengthTransferred = 0;
						WinUsb_ResetPipe(usbHandle, pipeId);
						std::cerr << "WinUsb_ReadPipe failure... lastResult: " << lastResult << std::endl;
					}
				}
				else
				{
					result = true;
				}
				return result;
			}

			bool usb_synchronous_read(unsigned char* buffer, int bufferLength, ULONG& lengthTransferred, DWORD TimeOut)
			{
				if (!winUsbOpen) throw std::runtime_error("winusb has not been initialized");

				auto result = false;

				BOOL bRetVal = true;
				OVERLAPPED hOvl;

				buffer[0] = buffer[1] = 0;

				hOvl.hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
				winusb::OnScopeExit closeHandle([=]{ CloseHandle(hOvl.hEvent); });

				bRetVal = WinUsb_ReadPipe(usbHandle, dataInPipe, buffer, bufferLength, &lengthTransferred, &hOvl);

				if (bRetVal)
					result = true;
				else
				{
					auto lastResult = GetLastError();
					if (lastResult == ERROR_IO_PENDING)
					{
						result = wait_for_async_transfer(hOvl, lengthTransferred, TimeOut, dataInPipe);
					}
					else
					{
						WinUsb_ResetPipe(usbHandle, dataInPipe);
						result = false;
					}
				}

				return result;
			}

			bool usb_synchronous_write(unsigned char* buffer, int bufferLength, unsigned char* outputBuffer, ULONG& bufferSize, DWORD TimeOut, bool oneDirection)
			{
				if (!winUsbOpen) throw std::runtime_error("winusb has not been initialized");

				auto result = false;
				OVERLAPPED hOvl;

				hOvl.hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
				winusb::OnScopeExit closeHandle([=]{ CloseHandle(hOvl.hEvent); });

				ULONG lengthWritten;
				auto bRetVal = WinUsb_WritePipe(usbHandle, dataOutPipe, buffer, bufferLength, &lengthWritten, &hOvl);
				if (bRetVal)
					result = true;
				else
				{
					auto lastError = GetLastError();
					if (lastError == ERROR_IO_PENDING)
					{
						result = wait_for_async_transfer(hOvl, lengthWritten, TimeOut, dataOutPipe);
					}
					else
					{
						WinUsb_ResetPipe(usbHandle, dataOutPipe);
						std::cerr << "WinUsb_ReadPipe failure... lastError: " << lastError << std::endl;
						result = false;
					}
				}

				if (!oneDirection && result == true)
					result = usb_synchronous_read(outputBuffer, 1024, bufferSize, 1000); // HW_MONITOR_BUFFER_SIZE

				return result;
			}

            IKsControl * get_control_node()
            {
                if(!ks_control)
                {       
                    com_ptr<IKsTopologyInfo> ks_topology_info = NULL;
                    check("QueryInterface", get_media_source(0)->QueryInterface(__uuidof(IKsTopologyInfo), (void **)&ks_topology_info));

                    DWORD num_nodes = 0;
                    check("get_numNodes", ks_topology_info->get_NumNodes(&num_nodes));

                    GUID node_type;
                    check("get_nodeType", ks_topology_info->get_NodeType(XUNODEID, &node_type));
                    const GUID KSNODETYPE_DEV_SPECIFIC_LOCAL{0x941C7AC0L, 0xC559, 0x11D0, {0x8A, 0x2B, 0x00, 0xA0, 0xC9, 0x25, 0x5A, 0xC1}};

                    if (node_type == KSNODETYPE_DEV_SPECIFIC_LOCAL)
                    {
                        com_ptr<IUnknown> unknown;
                        check("CreateNodeInstance", ks_topology_info->CreateNodeInstance(XUNODEID, IID_IUnknown, (LPVOID *)&unknown));
                        check("QueryInterface", unknown->QueryInterface(__uuidof(IKsControl), (void **)&ks_control));
                    }
                }
                if (!ks_control) throw std::runtime_error("unable to obtain control node");
                return ks_control;
            }
        };

        struct device_handle::_impl
        {
            const std::shared_ptr<device::_impl> parent;

            std::function<void(const void * frame)> callback;
            com_ptr<IMFMediaSource> media_source;
            com_ptr<IMFSourceReaderCallback> source_reader_callback;
            com_ptr<IMFSourceReader> source_reader;

            _impl(std::shared_ptr<device::_impl> parent) : parent(move(parent)) {}
        };

        ///////////////////
        // device_handle //
        ///////////////////

        static bool matches_format(const GUID & a, frame_format b)
        {
            return (a.Data1 == FCC('YUY2') && b == frame_format::YUYV)
                || (a.Data1 == FCC('Y8  ') && b == frame_format::Y8  )
                || (a.Data1 == FCC('Y12I') && b == frame_format::Y12I)
                || (a.Data1 == FCC('Z16 ') && b == frame_format::Z16 )
                || (a.Data1 == FCC('INVI') && b == frame_format::INVI)
                || (a.Data1 == FCC('INVR') && b == frame_format::INVR)
                || (a.Data1 == FCC('INRI') && b == frame_format::INRI);
            // TODO: Y16, Y8I, Y16I, other IVCAM formats, etc.
        }

        void device_handle::set_mode(int width, int height, frame_format cf, int fps)
        {
            for (DWORD j = 0; ; j++)
            {
                com_ptr<IMFMediaType> media_type;
                HRESULT hr = impl->source_reader->GetNativeMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, j, &media_type);
                if (hr == MF_E_NO_MORE_TYPES) break;
                check("IMFSourceReader::GetNativeMediaType", hr);

                UINT32 uvc_width, uvc_height, uvc_fps_num, uvc_fps_denom; GUID subtype;
                check("MFGetAttributeSize", MFGetAttributeSize(media_type, MF_MT_FRAME_SIZE, &uvc_width, &uvc_height));
                if(uvc_width != width || uvc_height != height) continue;

                check("IMFMediaType::GetGUID", media_type->GetGUID(MF_MT_SUBTYPE, &subtype));
                if(!matches_format(subtype, cf)) continue;

                check("MFGetAttributeRatio", MFGetAttributeRatio(media_type, MF_MT_FRAME_RATE, &uvc_fps_num, &uvc_fps_denom));
                if(uvc_fps_denom == 0) continue;
                int uvc_fps = uvc_fps_num / uvc_fps_denom;
                if(std::abs(fps - uvc_fps) > 1) continue;

                check("IMFSourceReader::SetCurrentMediaType", impl->source_reader->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, NULL, media_type));
                return;
            }
            throw std::runtime_error("no matching media type");
        }

        void device_handle::start_streaming(std::function<void(const void * frame)> callback)
        {
            impl->callback = callback;
            check("IMFSourceReader::ReadSample", impl->source_reader->ReadSample(MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, NULL, NULL, NULL, NULL));
        }

        void device_handle::stop_streaming()
        {
            throw std::runtime_error("device_handle::stop_streaming(...) not implemented");
        }

        ////////////
        // device //
        ////////////

        int device::get_vendor_id() const { return impl->vid; }
        int device::get_product_id() const { return impl->pid; }

        void device::get_control(uint8_t unit, uint8_t ctrl, void *data, int len) const
        {
            KSP_NODE node;
            memset(&node, 0, sizeof(KSP_NODE));
            node.Property.Set = {0x18682d34, 0xdd2c, 0x4073, {0xad, 0x23, 0x72, 0x14, 0x73, 0x9a, 0x07, 0x4c}}; // GUID_EXTENSION_UNIT_DESCRIPTOR
            node.Property.Id = ctrl;
            node.Property.Flags = KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_TOPOLOGY;
            node.NodeId = XUNODEID;

            ULONG bytes_received = 0;
            check("IKsControl::KsProperty", impl->get_control_node()->KsProperty((PKSPROPERTY)&node, sizeof(node), data, len, &bytes_received));
            if(bytes_received != len) throw std::runtime_error("XU read did not return enough data");
        }

        void device::set_control(uint8_t unit, uint8_t ctrl, void *data, int len)
        {               
            KSP_NODE node;
            memset(&node, 0, sizeof(KSP_NODE));
            node.Property.Set = {0x18682d34, 0xdd2c, 0x4073, {0xad, 0x23, 0x72, 0x14, 0x73, 0x9a, 0x07, 0x4c}}; // GUID_EXTENSION_UNIT_DESCRIPTOR
            node.Property.Id = ctrl;
            node.Property.Flags = KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_TOPOLOGY;
            node.NodeId = XUNODEID;
                
            check("IKsControl::KsProperty", impl->get_control_node()->KsProperty((PKSPROPERTY)&node, sizeof(KSP_NODE), data, len, nullptr));
        }

        void device::claim_interface(int interface_number)
        {
			impl->open_win_usb();
        }

        void device::bulk_transfer(unsigned char endpoint, unsigned char *data, int length, int *actual_length, unsigned int timeout)
        {		
			// #define IVCAM_MONITOR_ENDPOINT_OUT      0x1
			// #define IVCAM_MONITOR_ENDPOINT_IN       0x81

			// Write Operation
			if (endpoint == 0x1)
			{

			}

			// Read Operation
			else if (endpoint == 0x81)
			{

			}
        }

        class reader_callback : public IMFSourceReaderCallback
        {
            std::weak_ptr<device_handle::_impl> owner; // The device_handle holds a reference to us, so use weak_ptr to prevent a cycle
            ULONG ref_count;
        public:
            reader_callback(std::shared_ptr<device_handle::_impl> owner) : owner(owner), ref_count() {}

            // Implement IUnknown
            HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void ** ppvObject) override 
            {
                static const QITAB table[] = {QITABENT(reader_callback, IUnknown), QITABENT(reader_callback, IMFSourceReaderCallback), {0}};
                return QISearch(this, table, riid, ppvObject);
            }
            ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&ref_count); }
            ULONG STDMETHODCALLTYPE Release() override 
            { 
                ULONG count = InterlockedDecrement(&ref_count);
                if(count == 0) delete this;
                return count;
            }

            // Implement IMFSourceReaderCallback
            HRESULT STDMETHODCALLTYPE OnReadSample(HRESULT hrStatus, DWORD dwStreamIndex, DWORD dwStreamFlags, LONGLONG llTimestamp, IMFSample * sample) override 
            { 
                if(auto owner_ptr = owner.lock())
                {
                    if(sample)
                    {
                        com_ptr<IMFMediaBuffer> buffer = NULL;
                        if(SUCCEEDED(sample->GetBufferByIndex(0, &buffer)))
                        {
                            BYTE * byte_buffer; DWORD max_length, current_length;
                            if(SUCCEEDED(buffer->Lock(&byte_buffer, &max_length, &current_length)))
                            {
                                 owner_ptr->callback(byte_buffer);
                            }
                        }
                    }

                    HRESULT hr = owner_ptr->source_reader->ReadSample(MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, NULL, NULL, NULL, NULL);
                }
                return S_OK; 
            }
            HRESULT STDMETHODCALLTYPE OnFlush(DWORD dwStreamIndex) override { return S_OK; }
            HRESULT STDMETHODCALLTYPE OnEvent(DWORD dwStreamIndex, IMFMediaEvent *pEvent) override { return S_OK; }
        };

        device_handle device::claim_subdevice(int subdevice_index)
        {
            auto sub = std::make_shared<device_handle::_impl>(impl);

            sub->media_source = impl->get_media_source(subdevice_index);
            sub->source_reader_callback = new reader_callback(sub);

            com_ptr<IMFAttributes> pAttributes;
            check("MFCreateAttributes", MFCreateAttributes(&pAttributes, 1));
            check("IMFAttributes::SetUnknown", pAttributes->SetUnknown(MF_SOURCE_READER_ASYNC_CALLBACK, static_cast<IUnknown *>(sub->source_reader_callback))); // TODO: Fix
            check("MFCreateSourceReaderFromMediaSource", MFCreateSourceReaderFromMediaSource(sub->media_source, pAttributes, &sub->source_reader));
            
            return {sub};
        }

        /////////////
        // context //
        /////////////

        context context::create()
        {
            return {std::make_shared<context::_impl>()};
        }

        std::vector<std::string> tokenize(std::string string, char separator)
        {
            std::vector<std::string> tokens;
            std::string::size_type i1 = 0;
            while(true)
            {
                auto i2 = string.find(separator, i1);
                if(i2 == std::string::npos)
                {
                    tokens.push_back(string.substr(i1));
                    return tokens;
                }
                tokens.push_back(string.substr(i1, i2-i1));
                i1 = i2+1;
            }
        }

        std::vector<device> context::query_devices()
        {
            IMFAttributes * pAttributes = NULL;
            check("MFCreateAttributes", MFCreateAttributes(&pAttributes, 1));
            check("IMFAttributes::SetGUID", pAttributes->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID));
 
            IMFActivate ** ppDevices;
            UINT32 numDevices;
            check("MFEnumDeviceSources", MFEnumDeviceSources(pAttributes, &ppDevices, &numDevices));

            std::vector<device> devices;
            for(UINT32 i=0; i<numDevices; ++i)
            {
                com_ptr<IMFActivate> pDevice;
                *&pDevice = ppDevices[i];

                WCHAR * wchar_name = NULL;
                UINT32 length;
                pDevice->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK, &wchar_name, &length);
                auto name = win_to_utf(wchar_name);
                CoTaskMemFree(wchar_name);

                int vid, pid, mi;

                auto tokens = tokenize(name, '#');
                if(tokens.size() < 1 || tokens[0] != R"(\\?\usb)") continue; // Not a USB device
                if(tokens.size() < 3)
                {
                    std::cerr << "malformed usb device path: " << name << std::endl;
                    continue;
                }

                auto ids = tokenize(tokens[1], '&');
                if(ids[0].size() != 8 || ids[0].substr(0,4) != "vid_" || !(std::istringstream(ids[0].substr(4,4)) >> std::hex >> vid))
                {
                    std::cerr << "malformed vid string: " << tokens[1] << std::endl;
                    continue;
                }

                if(ids[1].size() != 8 || ids[1].substr(0,4) != "pid_" || !(std::istringstream(ids[1].substr(4,4)) >> std::hex >> pid))
                {
                    std::cerr << "malformed pid string: " << tokens[1] << std::endl;
                    continue;
                }

                if(ids[2].size() != 5 || ids[2].substr(0,3) != "mi_" || !(std::istringstream(ids[2].substr(3,2)) >> mi))
                {
                    std::cerr << "malformed mi string: " << tokens[1] << std::endl;
                    continue;
                }

                ids = tokenize(tokens[2], '&');
                if(ids.size() < 2)
                {
                    std::cerr << "malformed id string: " << tokens[2] << std::endl;
                    continue;               
                }
                std::string unique_id = ids[1];

                device dev;
                for(auto & d : devices)
                {
                    if(d.impl->vid == vid && d.impl->pid == pid && d.impl->unique_id == unique_id)
                    {
                        dev = d;
                    }
                }
                if(!dev)
                {
                    dev = {std::make_shared<device::_impl>(impl, vid, pid, unique_id)};
                    devices.push_back(dev);
                }

                int subdevice_index = mi/2;
                if(subdevice_index >= dev.impl->subdevices.size()) dev.impl->subdevices.resize(subdevice_index+1);
                dev.impl->subdevices[subdevice_index] = pDevice;
            }
            CoTaskMemFree(ppDevices);
            for(auto & dev : devices) dev.impl->sources.resize(dev.impl->subdevices.size());
            return devices;
        }
    }
}
#endif
