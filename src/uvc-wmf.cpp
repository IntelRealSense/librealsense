// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.
#include <chrono>
#include <iostream>


#ifdef RS_USE_WMF_BACKEND

#if (_MSC_FULL_VER < 180031101)
    #error At least Visual Studio 2013 Update 4 is required to compile this backend
#endif

#include <windows.h>
#include <usbioctl.h>
#include <sstream>

#include "uvc.h"

#include <Shlwapi.h>        // For QISearch, etc.
#include <mfapi.h>          // For MFStartup, etc.
#include <mfidl.h>          // For MF_DEVSOURCE_*, etc.
#include <mfreadwrite.h>    // MFCreateSourceReaderFromMediaSource
#include <mferror.h>
#include "hw-monitor.h"

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

#include <Cfgmgr32.h>

#pragma comment(lib, "cfgmgr32.lib")

#include <SetupAPI.h>
#include <WinUsb.h>

#include <functional>
#include <thread>
#include <chrono>
#include <algorithm>
#include <regex>
#include <map>

#include <strsafe.h>

DEFINE_GUID(GUID_DEVINTERFACE_USB_DEVICE, 0xA5DCBF10L, 0x6530, 0x11D2, 0x90, 0x1F, 0x00, \
    0xC0, 0x4F, 0xB9, 0x51, 0xED);
DEFINE_GUID(GUID_DEVINTERFACE_IMAGE, 0x6bdd1fc6L, 0x810f, 0x11d0, 0xbe, 0xc7, 0x08, 0x00, \
    0x2b, 0xe2, 0x09, 0x2f);

namespace rsimpl
{
    namespace uvc
    {
        const auto FISHEYE_HWMONITOR_INTERFACE = 2;
        const uvc::guid FISHEYE_WIN_USB_DEVICE_GUID = { 0xC0B55A29, 0xD7B6, 0x436E, { 0xA6, 0xEF, 0x2E, 0x76, 0xED, 0x0A, 0xBC, 0xA5 } };
        // Translation of user-provided fourcc code into device supported one:           Note the Big-Endian notation
        const std::map<uint32_t, uint32_t> fourcc_map = { { 0x47524559, 0x59382020 },       /* 'GREY' => 'Y8  '. */
                                                          { 0x70524141, 0x52573130 } };    /* 'RW10' => 'pRAA'. */

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

        bool parse_usb_path(int & vid, int & pid, int & mi, std::string & unique_id, const std::string & path)
        {
            auto name = path;
            std::transform(begin(name), end(name), begin(name), ::tolower);
            auto tokens = tokenize(name, '#');
            if(tokens.size() < 1 || tokens[0] != R"(\\?\usb)") return false; // Not a USB device
            if(tokens.size() < 3)
            {
                LOG_ERROR("malformed usb device path: " << name);
                return false;
            }

            auto ids = tokenize(tokens[1], '&');
            if(ids[0].size() != 8 || ids[0].substr(0,4) != "vid_" || !(std::istringstream(ids[0].substr(4,4)) >> std::hex >> vid))
            {
                LOG_ERROR("malformed vid string: " << tokens[1]);
                return false;
            }

            if(ids[1].size() != 8 || ids[1].substr(0,4) != "pid_" || !(std::istringstream(ids[1].substr(4,4)) >> std::hex >> pid))
            {
                LOG_ERROR("malformed pid string: " << tokens[1]);
                return false;
            }

            if(ids[2].size() != 5 || ids[2].substr(0,3) != "mi_" || !(std::istringstream(ids[2].substr(3,2)) >> mi))
            {
                LOG_ERROR("malformed mi string: " << tokens[1]);
                return false;
            }

            ids = tokenize(tokens[2], '&');
            if(ids.size() < 2)
            {
                LOG_ERROR("malformed id string: " << tokens[2]);
                return false;
            }
            unique_id = ids[1];
            return true;
        }

        bool parse_usb_path_from_device_id(int & vid, int & pid, int & mi, std::string & unique_id, const std::string & device_id)
        {
            auto name = device_id;
            std::transform(begin(name), end(name), begin(name), ::tolower);
            auto tokens = tokenize(name, '\\');
            if (tokens.size() < 1 || tokens[0] != R"(usb)") return false; // Not a USB device

            auto ids = tokenize(tokens[1], '&');
            if (ids[0].size() != 8 || ids[0].substr(0, 4) != "vid_" || !(std::istringstream(ids[0].substr(4, 4)) >> std::hex >> vid))
            {
                LOG_ERROR("malformed vid string: " << tokens[1]);
                return false;
            }

            if (ids[1].size() != 8 || ids[1].substr(0, 4) != "pid_" || !(std::istringstream(ids[1].substr(4, 4)) >> std::hex >> pid))
            {
                LOG_ERROR("malformed pid string: " << tokens[1]);
                return false;
            }

            if (ids[2].size() != 5 || ids[2].substr(0, 3) != "mi_" || !(std::istringstream(ids[2].substr(3, 2)) >> mi))
            {
                LOG_ERROR("malformed mi string: " << tokens[1]);
                return false;
            }

            ids = tokenize(tokens[2], '&');
            if (ids.size() < 2)
            {
                LOG_ERROR("malformed id string: " << tokens[2]);
                return false;
            }
            unique_id = ids[1];
            return true;
        }


        struct context
        {
            context()
            {
                CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                MFStartup(MF_VERSION, MFSTARTUP_NOSOCKET);
            }
            ~context()
            {   
                MFShutdown();
                CoUninitialize();
            }
        };

        class reader_callback : public IMFSourceReaderCallback
        {
            std::weak_ptr<device> owner; // The device holds a reference to us, so use weak_ptr to prevent a cycle
            int subdevice_index;
            ULONG ref_count;
            volatile bool streaming = false;
        public:
            reader_callback(std::weak_ptr<device> owner, int subdevice_index) : owner(owner), subdevice_index(subdevice_index), ref_count() {}

            bool is_streaming() const { return streaming; }
            void on_start() { streaming = true; }

#pragma warning( push )
#pragma warning( disable: 4838 )
            // Implement IUnknown
            HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void ** ppvObject) override 
            {
                static const QITAB table[] = {QITABENT(reader_callback, IUnknown), QITABENT(reader_callback, IMFSourceReaderCallback), {0}};
                return QISearch(this, table, riid, ppvObject);
            }
#pragma warning( pop )

            ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&ref_count); }
            ULONG STDMETHODCALLTYPE Release() override 
            { 
                ULONG count = InterlockedDecrement(&ref_count);
                if(count == 0) delete this;
                return count;
            }

            // Implement IMFSourceReaderCallback
            HRESULT STDMETHODCALLTYPE OnReadSample(HRESULT hrStatus, DWORD dwStreamIndex, DWORD dwStreamFlags, LONGLONG llTimestamp, IMFSample * sample) override;
            HRESULT STDMETHODCALLTYPE OnFlush(DWORD dwStreamIndex) override { streaming = false; return S_OK; }
            HRESULT STDMETHODCALLTYPE OnEvent(DWORD dwStreamIndex, IMFMediaEvent *pEvent) override { return S_OK; }
        };

        struct subdevice
        {
            com_ptr<reader_callback> reader_callback;
            com_ptr<IMFActivate> mf_activate;
            com_ptr<IMFMediaSource> mf_media_source;
            com_ptr<IAMCameraControl> am_camera_control;
            com_ptr<IAMVideoProcAmp> am_video_proc_amp;
            std::map<int, com_ptr<IKsControl>> ks_controls;
            com_ptr<IMFSourceReader> mf_source_reader;
            video_channel_callback callback = nullptr;
            data_channel_callback  channel_data_callback = nullptr;
            int vid, pid;

            void set_data_channel_cfg(data_channel_callback callback)
            {
                this->channel_data_callback = callback;
            }

            com_ptr<IMFMediaSource> get_media_source()
            {
                if(!mf_media_source)
                {
                    check("IMFActivate::ActivateObject", mf_activate->ActivateObject(__uuidof(IMFMediaSource), (void **)&mf_media_source));
                    if (mf_media_source)
                    {
                        check("IMFMediaSource::QueryInterface", mf_media_source->QueryInterface(__uuidof(IAMCameraControl), (void **)&am_camera_control));
                        if (SUCCEEDED(mf_media_source->QueryInterface(__uuidof(IAMVideoProcAmp), (void **)&am_video_proc_amp))) LOG_DEBUG("obtained IAMVideoProcAmp");
                    }
                    else throw std::runtime_error(to_string() << "Invalid media source");
                }
                return mf_media_source;

            }

            static bool wait_for_async_operation(WINUSB_INTERFACE_HANDLE interfaceHandle, OVERLAPPED &hOvl, ULONG &lengthTransferred, USHORT timeout)
            {
                if (GetOverlappedResult(interfaceHandle, &hOvl, &lengthTransferred, FALSE))
                    return true;

                auto lastResult = GetLastError();
                if (lastResult == ERROR_IO_PENDING || lastResult == ERROR_IO_INCOMPLETE)
                {
                    WaitForSingleObject(hOvl.hEvent, timeout);
                    auto res = GetOverlappedResult(interfaceHandle, &hOvl, &lengthTransferred, FALSE);
                    if (res != 1)
                    {
                        return false;
                    }
                }
                else
                {
                    lengthTransferred = 0;
                    WinUsb_ResetPipe(interfaceHandle, 0x84);
                    return false;
                }

                return true;
            }

            class safe_handle
            {
            public:
                safe_handle(HANDLE handle) :_handle(handle)
                {

                }

                ~safe_handle()
                {
                    if (_handle != nullptr)
                    {
                        CloseHandle(_handle);
                        _handle = nullptr;
                    }
                }

                bool Set()
                {
                    if (_handle == nullptr) return false;
                    SetEvent(_handle);
                    return true;
                }

                bool Wait(DWORD timeout) const
                {
                    if (_handle == nullptr) return false;

                    return WaitForSingleObject(_handle, timeout) == WAIT_OBJECT_0; // Return true only if object was signaled
                }


                HANDLE GetHandle() const { return _handle; }
            private:
                safe_handle() = delete;

                // Disallow copy:
                safe_handle(const safe_handle&) = delete;
                safe_handle& operator=(const safe_handle&) = delete;

                HANDLE _handle;
            };

            static void poll_interrupts(HANDLE *handle, const std::vector<subdevice *> & subdevices,uint16_t timeout)
            {
                static const unsigned short interrupt_buf_size = 0x400;
                uint8_t buffer[interrupt_buf_size];                         /* 64 byte transfer buffer  - dedicated channel*/
                ULONG num_bytes = 0;                                        /* Actual bytes transferred. */
                OVERLAPPED hOvl;
                safe_handle sh(CreateEvent(nullptr, false, false, nullptr));
                hOvl.hEvent = sh.GetHandle();
                // TODO - replace hard-coded values : 0x82 and 1000
                int res = WinUsb_ReadPipe(*handle, 0x84, buffer, interrupt_buf_size, &num_bytes, &hOvl);
                if (0 == res)
                {
                    auto lastError = GetLastError();
                    if (lastError == ERROR_IO_PENDING)
                    {
                        auto sts = wait_for_async_operation(*handle, hOvl, num_bytes, timeout);
                        lastError = GetLastError();
                        if (lastError == ERROR_OPERATION_ABORTED)
                        {
                            perror("receiving interrupt_ep bytes failed");
                            fprintf(stderr, "Error receiving message.\n");
                        }
                        if (!sts)
                            return;
                    }
                    else
                    {
                        WinUsb_ResetPipe(*handle, 0x84);
                        perror("receiving interrupt_ep bytes failed");
                        fprintf(stderr, "Error receiving message.\n");
                        return;
                    }

                    if (num_bytes == 0)
                        return;

                    // Propagate the data to device layer
                    for(auto & sub : subdevices)
                        if (sub->channel_data_callback)
                            sub->channel_data_callback(buffer, (unsigned long)num_bytes);
                }
                else
                {
                    // todo exception
                    perror("receiving interrupt_ep bytes failed");
                    fprintf(stderr, "Error receiving message.\n");
                }
            }

            IKsControl * get_ks_control(const uvc::extension_unit & xu)
            {
                auto it = ks_controls.find(xu.node);
                if(it != end(ks_controls)) return it->second;

                get_media_source();

                // Attempt to retrieve IKsControl
                com_ptr<IKsTopologyInfo> ks_topology_info = NULL;
                check("QueryInterface", mf_media_source->QueryInterface(__uuidof(IKsTopologyInfo), (void **)&ks_topology_info));

                GUID node_type;
                check("get_NodeType", ks_topology_info->get_NodeType(xu.node, &node_type));
                const GUID KSNODETYPE_DEV_SPECIFIC_LOCAL{0x941C7AC0L, 0xC559, 0x11D0, {0x8A, 0x2B, 0x00, 0xA0, 0xC9, 0x25, 0x5A, 0xC1}};
                if(node_type != KSNODETYPE_DEV_SPECIFIC_LOCAL) throw std::runtime_error(to_string() << "Invalid extension unit node ID: " << xu.node);

                com_ptr<IUnknown> unknown;
                check("CreateNodeInstance", ks_topology_info->CreateNodeInstance(xu.node, IID_IUnknown, (LPVOID *)&unknown));

                com_ptr<IKsControl> ks_control;
                check("QueryInterface", unknown->QueryInterface(__uuidof(IKsControl), (void **)&ks_control));
                LOG_INFO("Obtained KS control node " << xu.node);
                return ks_controls[xu.node] = ks_control;
            }
        };

        struct device
        {
            const std::shared_ptr<context> parent;
            const int vid, pid;
            const std::string unique_id;

            std::vector<subdevice> subdevices;

            HANDLE usb_file_handle = INVALID_HANDLE_VALUE;
            WINUSB_INTERFACE_HANDLE usb_interface_handle = INVALID_HANDLE_VALUE;

            std::vector<int> claimed_interfaces;

            int aux_vid, aux_pid;
            std::string aux_unique_id;
            std::thread data_channel_thread;
            volatile bool data_stop;

            device(std::shared_ptr<context> parent, int vid, int pid, std::string unique_id) : parent(move(parent)), vid(vid), pid(pid), unique_id(move(unique_id)), aux_pid(0), aux_vid(0), data_stop(false)
            {
            }

            ~device() { stop_streaming(); stop_data_acquisition(); close_win_usb(); }

            IKsControl * get_ks_control(const uvc::extension_unit & xu)
            {
                return subdevices[xu.subdevice].get_ks_control(xu);
            }

            void start_data_acquisition()
            {
                std::vector<subdevice *> data_channel_subs;
                for (auto & sub : subdevices)
                {
                    if (sub.channel_data_callback)
                    {
                        data_channel_subs.push_back(&sub);
                    }
                }

                if (claimed_interfaces.size())
                {
                    data_channel_thread = std::thread([this, data_channel_subs]()
                    {
                        // Polling
                        while (!data_stop)
                        {
                            subdevice::poll_interrupts(&usb_interface_handle, data_channel_subs, 100);
                        }
                    });
                }
            }

            void stop_data_acquisition()
            {
                if (data_channel_thread.joinable())
                {
                    data_stop = true;
                    data_channel_thread.join();
                    data_stop = false;
                }
            }

            void start_streaming()
            {
                for(auto & sub : subdevices)
                {
                    if(sub.mf_source_reader)
                    {
                        sub.reader_callback->on_start();
                        check("IMFSourceReader::ReadSample", sub.mf_source_reader->ReadSample(MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, NULL, NULL, NULL, NULL));                    
                    }
                }
            }

            void stop_streaming()
            {
                for(auto & sub : subdevices)
                {
                    if(sub.mf_source_reader) sub.mf_source_reader->Flush(MF_SOURCE_READER_FIRST_VIDEO_STREAM);
                }
                while(true)
                {
                    bool is_streaming = false;
                    for(auto & sub : subdevices) is_streaming |= sub.reader_callback->is_streaming();                   
                    if(is_streaming) std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    else break;
                }

                // Free up our source readers, our KS control nodes, and our media sources, but retain our original IMFActivate objects for later reuse
                for(auto & sub : subdevices)
                {
                    sub.mf_source_reader = nullptr;
                    sub.am_camera_control = nullptr;
                    sub.am_video_proc_amp = nullptr;
                    sub.ks_controls.clear();
                    if(sub.mf_media_source)
                    {
                        sub.mf_media_source = nullptr;
                        check("IMFActivate::ShutdownObject", sub.mf_activate->ShutdownObject());
                    }
                    sub.callback = {};
                }
            }

            com_ptr<IMFMediaSource> get_media_source(int subdevice_index)
            {
                return subdevices[subdevice_index].get_media_source();
            }           

            void open_win_usb(int vid, int pid, std::string unique_id, const guid & interface_guid, int interface_number) try
            {    
                static_assert(sizeof(guid) == sizeof(GUID), "struct packing error");
                HDEVINFO device_info = SetupDiGetClassDevs((const GUID *)&interface_guid, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
                if (device_info == INVALID_HANDLE_VALUE) throw std::runtime_error("SetupDiGetClassDevs");
                auto di = std::shared_ptr<void>(device_info, SetupDiDestroyDeviceInfoList);

                for(int member_index = 0; ; ++member_index)
                {
                    // Enumerate all the device interfaces in the device information set.
                    SP_DEVICE_INTERFACE_DATA interfaceData = {sizeof(SP_DEVICE_INTERFACE_DATA)};
                    if(SetupDiEnumDeviceInterfaces(device_info, nullptr, (const GUID *)&interface_guid, member_index, &interfaceData) == FALSE)
                    {
                        if(GetLastError() == ERROR_NO_MORE_ITEMS) break;
                        continue;
                    }                           

                    // Allocate space for a detail data struct
                    unsigned long detail_data_size = 0;
                    SetupDiGetDeviceInterfaceDetail(device_info, &interfaceData, nullptr, 0, &detail_data_size, nullptr);
                    if(GetLastError() != ERROR_INSUFFICIENT_BUFFER)
                    {
                        LOG_ERROR("SetupDiGetDeviceInterfaceDetail failed");
                        continue;
                    }
                    auto alloc = std::malloc(detail_data_size);
                    if(!alloc) throw std::bad_alloc();

                    // Retrieve the detail data struct
                    auto detail_data = std::shared_ptr<SP_DEVICE_INTERFACE_DETAIL_DATA>(reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA *>(alloc), std::free);
                    detail_data->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
                    if (!SetupDiGetDeviceInterfaceDetail(device_info, &interfaceData, detail_data.get(), detail_data_size, nullptr, nullptr))
                    {
                        LOG_ERROR("SetupDiGetDeviceInterfaceDetail failed");
                        continue;
                    }
                    if (detail_data->DevicePath == nullptr) continue;
                    // Check if this is our device
                    int usb_vid, usb_pid, usb_mi; std::string usb_unique_id;
                    if(!parse_usb_path(usb_vid, usb_pid, usb_mi, usb_unique_id, win_to_utf(detail_data->DevicePath))) continue;
                    if(usb_vid != vid || usb_pid != pid || usb_mi != interface_number || usb_unique_id != unique_id) continue;                    

                    HANDLE* file_handle = nullptr;
                    WINUSB_INTERFACE_HANDLE* usb_handle = nullptr;

                    file_handle = &usb_file_handle;
                    usb_handle = &usb_interface_handle;

                    *file_handle = CreateFile(detail_data->DevicePath, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_WRITE | FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, nullptr);
                    if (*file_handle == INVALID_HANDLE_VALUE) throw std::runtime_error("CreateFile(...) failed");

                    if (!WinUsb_Initialize(*file_handle, usb_handle))
                    {
                        LOG_ERROR("Last Error: " << GetLastError());
                        throw std::runtime_error("could not initialize winusb");
                    }

                    // We successfully set up a WinUsb interface handle to our device
                    return;
                }
                throw std::runtime_error("Unable to open device via WinUSB");
            }
            catch(...)
            {
                close_win_usb();
                throw;
            }

            void close_win_usb()
            {
                if (usb_interface_handle != INVALID_HANDLE_VALUE)
                {
                    WinUsb_Free(usb_interface_handle);
                    usb_interface_handle = INVALID_HANDLE_VALUE;
                }

                if(usb_file_handle != INVALID_HANDLE_VALUE)
                {
                    CloseHandle(usb_file_handle);
                    usb_file_handle = INVALID_HANDLE_VALUE;
                }
            }

            bool usb_synchronous_read(uint8_t endpoint, void * buffer, int bufferLength, int * actual_length, DWORD TimeOut)
            {
                if (usb_interface_handle == INVALID_HANDLE_VALUE) throw std::runtime_error("winusb has not been initialized");

                auto result = false;

                BOOL bRetVal = true;
                
                ULONG lengthTransferred;

                bRetVal = WinUsb_ReadPipe(usb_interface_handle, endpoint, (PUCHAR)buffer, bufferLength, &lengthTransferred, NULL);

                if (bRetVal)
                    result = true;
                else
                {
                    auto lastResult = GetLastError();
                    WinUsb_ResetPipe(usb_interface_handle, endpoint);
                    result = false;
                }

                *actual_length = lengthTransferred;
                return result;
            }

            bool usb_synchronous_write(uint8_t endpoint, void * buffer, int bufferLength, DWORD TimeOut)
            {
                if (usb_interface_handle == INVALID_HANDLE_VALUE) throw std::runtime_error("winusb has not been initialized");

                auto result = false;

                ULONG lengthWritten;
                auto bRetVal = WinUsb_WritePipe(usb_interface_handle, endpoint, (PUCHAR)buffer, bufferLength, &lengthWritten, NULL);
                if (bRetVal)
                    result = true;
                else
                {
                    auto lastError = GetLastError();
                    WinUsb_ResetPipe(usb_interface_handle, endpoint);
                    LOG_ERROR("WinUsb_ReadPipe failure... lastError: " << lastError);
                    result = false;
                }

                return result;
            }
        };

        HRESULT reader_callback::OnReadSample(HRESULT hrStatus, DWORD dwStreamIndex, DWORD dwStreamFlags, LONGLONG llTimestamp, IMFSample * sample) 
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
                            auto continuation = [buffer, this]()
                            {
                                buffer->Unlock();
                            };

                            owner_ptr->subdevices[subdevice_index].callback(byte_buffer, continuation);
                        }
                    }
                }

                if (auto owner_ptr_new = owner.lock())
                {
                    auto hr = owner_ptr_new->subdevices[subdevice_index].mf_source_reader->ReadSample(MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, NULL, NULL, NULL, NULL);
                    switch (hr)
                    {
                    case S_OK: break;
                    case MF_E_INVALIDREQUEST: LOG_ERROR("ReadSample returned MF_E_INVALIDREQUEST"); break;
                    case MF_E_INVALIDSTREAMNUMBER: LOG_ERROR("ReadSample returned MF_E_INVALIDSTREAMNUMBER"); break;
                    case MF_E_NOTACCEPTING: LOG_ERROR("ReadSample returned MF_E_NOTACCEPTING"); break;
                    case E_INVALIDARG: LOG_ERROR("ReadSample returned E_INVALIDARG"); break;
                    case MF_E_VIDEO_RECORDING_DEVICE_INVALIDATED: LOG_ERROR("ReadSample returned MF_E_VIDEO_RECORDING_DEVICE_INVALIDATED"); break;
                    default: LOG_ERROR("ReadSample returned HRESULT " << std::hex << (uint32_t)hr); break;
                    }
                    if (hr != S_OK) streaming = false;
                }
            }
            return S_OK; 
        }

        ////////////
        // device //
        ////////////

        int get_vendor_id(const device & device) { return device.vid; }
        int get_product_id(const device & device) { return device.pid; }

        void get_control(const device & device, const extension_unit & xu, uint8_t ctrl, void *data, int len)
        {
            auto ks_control = const_cast<uvc::device &>(device).get_ks_control(xu);

            KSP_NODE node;
            memset(&node, 0, sizeof(KSP_NODE));
            node.Property.Set = reinterpret_cast<const GUID &>(xu.id);
            node.Property.Id = ctrl;
            node.Property.Flags = KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_TOPOLOGY;
            node.NodeId = xu.node;

            ULONG bytes_received = 0;
            check("IKsControl::KsProperty", ks_control->KsProperty((PKSPROPERTY)&node, sizeof(node), data, len, &bytes_received));
            if(bytes_received != len) throw std::runtime_error("XU read did not return enough data");
        }

        void set_control(device & device, const extension_unit & xu, uint8_t ctrl, void *data, int len)
        {        
            auto ks_control = device.get_ks_control(xu);

            KSP_NODE node;
            memset(&node, 0, sizeof(KSP_NODE));
            node.Property.Set = reinterpret_cast<const GUID &>(xu.id);
            node.Property.Id = ctrl;
            node.Property.Flags = KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_TOPOLOGY;
            node.NodeId = xu.node;
                
            ULONG bytes_received = 0;
            check("IKsControl::KsProperty", ks_control->KsProperty((PKSPROPERTY)&node, sizeof(KSP_NODE), data, len, &bytes_received));
        }

        void claim_interface(device & device, const guid & interface_guid, int interface_number)
        {
            device.open_win_usb(device.vid, device.pid, device.unique_id, interface_guid, interface_number);
            device.claimed_interfaces.push_back(interface_number);
        }

        void claim_aux_interface(device & device, const guid & interface_guid, int interface_number)
        {
            device.open_win_usb(device.aux_vid, device.aux_pid, device.aux_unique_id, interface_guid, interface_number);
            device.claimed_interfaces.push_back(interface_number);
        }

        void bulk_transfer(device & device, uint8_t endpoint, void * data, int length, int *actual_length, unsigned int timeout)
        {
            if(USB_ENDPOINT_DIRECTION_OUT(endpoint))
            {
                device.usb_synchronous_write(endpoint, data, length, timeout);
            }
            
            if(USB_ENDPOINT_DIRECTION_IN(endpoint))
            {
                device.usb_synchronous_read(endpoint, data, length, actual_length, timeout);
            }
        }

        void set_subdevice_mode(device & device, int subdevice_index, int width, int height, uint32_t fourcc, int fps, video_channel_callback callback)
        {
            auto & sub = device.subdevices[subdevice_index];
            
            if(!sub.mf_source_reader)
            {
                com_ptr<IMFAttributes> pAttributes;
                check("MFCreateAttributes", MFCreateAttributes(&pAttributes, 1));
                check("IMFAttributes::SetUnknown", pAttributes->SetUnknown(MF_SOURCE_READER_ASYNC_CALLBACK, static_cast<IUnknown *>(sub.reader_callback)));
                check("MFCreateSourceReaderFromMediaSource", MFCreateSourceReaderFromMediaSource(sub.get_media_source(), pAttributes, &sub.mf_source_reader));
            }

            if (fourcc_map.count(fourcc))   fourcc = fourcc_map.at(fourcc);

            for (DWORD j = 0; ; j++)
            {
                com_ptr<IMFMediaType> media_type;
                HRESULT hr = sub.mf_source_reader->GetNativeMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, j, &media_type);
                if (hr == MF_E_NO_MORE_TYPES) break;
                check("IMFSourceReader::GetNativeMediaType", hr);

                UINT32 uvc_width, uvc_height, uvc_fps_num, uvc_fps_denom; GUID subtype;
                check("MFGetAttributeSize", MFGetAttributeSize(media_type, MF_MT_FRAME_SIZE, &uvc_width, &uvc_height));
                if(uvc_width != width || uvc_height != height) continue;

                check("IMFMediaType::GetGUID", media_type->GetGUID(MF_MT_SUBTYPE, &subtype));
                if(reinterpret_cast<const big_endian<uint32_t> &>(subtype.Data1) != fourcc) continue;

                check("MFGetAttributeRatio", MFGetAttributeRatio(media_type, MF_MT_FRAME_RATE, &uvc_fps_num, &uvc_fps_denom));
                if(uvc_fps_denom == 0) continue;
                int uvc_fps = uvc_fps_num / uvc_fps_denom;
                if(std::abs(fps - uvc_fps) > 1) continue;

                check("IMFSourceReader::SetCurrentMediaType", sub.mf_source_reader->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, NULL, media_type));
                sub.callback = callback;
                return;
            }
            throw std::runtime_error(to_string() << "no matching media type for  pixel format " << std::hex << fourcc);
        }

        void set_subdevice_data_channel_handler(device & device, int subdevice_index, data_channel_callback callback)
        {           
            device.subdevices[subdevice_index].set_data_channel_cfg(callback);
        }

        void start_streaming(device & device, int num_transfer_bufs) { device.start_streaming(); }
        void stop_streaming(device & device) { device.stop_streaming(); }

        void start_data_acquisition(device & device)
        {
            device.start_data_acquisition();
        }

        void stop_data_acquisition(device & device)
        {
            device.stop_data_acquisition();
        }

        struct pu_control { rs_option option; long property; bool enable_auto; };
        static const pu_control pu_controls[] = {
            {RS_OPTION_COLOR_BACKLIGHT_COMPENSATION, VideoProcAmp_BacklightCompensation},
            {RS_OPTION_COLOR_BRIGHTNESS, VideoProcAmp_Brightness},
            {RS_OPTION_COLOR_CONTRAST, VideoProcAmp_Contrast},
            {RS_OPTION_COLOR_GAIN, VideoProcAmp_Gain},
            {RS_OPTION_COLOR_GAMMA, VideoProcAmp_Gamma},
            {RS_OPTION_COLOR_HUE, VideoProcAmp_Hue},
            {RS_OPTION_COLOR_SATURATION, VideoProcAmp_Saturation},
            {RS_OPTION_COLOR_SHARPNESS, VideoProcAmp_Sharpness},
            {RS_OPTION_COLOR_WHITE_BALANCE, VideoProcAmp_WhiteBalance},
            {RS_OPTION_COLOR_ENABLE_AUTO_WHITE_BALANCE, VideoProcAmp_WhiteBalance, true},
            {RS_OPTION_FISHEYE_GAIN, VideoProcAmp_Gain}
        };

        void set_pu_control(device & device, int subdevice, rs_option option, int value)
        {
            auto & sub = device.subdevices[subdevice];
            sub.get_media_source();
            if (option == RS_OPTION_COLOR_EXPOSURE)
            {
                check("IAMCameraControl::Set", sub.am_camera_control->Set(CameraControl_Exposure, static_cast<int>(value), CameraControl_Flags_Manual));
                return;
            }
            if(option == RS_OPTION_COLOR_ENABLE_AUTO_EXPOSURE)
            {
                if(value) check("IAMCameraControl::Set", sub.am_camera_control->Set(CameraControl_Exposure, 0, CameraControl_Flags_Auto));
                else
                {
                    long min, max, step, def, caps;
                    check("IAMCameraControl::GetRange", sub.am_camera_control->GetRange(CameraControl_Exposure, &min, &max, &step, &def, &caps));
                    check("IAMCameraControl::Set", sub.am_camera_control->Set(CameraControl_Exposure, def, CameraControl_Flags_Manual));
                }
                return;
            }
            for(auto & pu : pu_controls)
            {
                if(option == pu.option)
                {
                    if(pu.enable_auto)
                    {
                        if(value) check("IAMVideoProcAmp::Set", sub.am_video_proc_amp->Set(pu.property, 0, VideoProcAmp_Flags_Auto));
                        else
                        {
                            long min, max, step, def, caps;
                            check("IAMVideoProcAmp::GetRange", sub.am_video_proc_amp->GetRange(pu.property, &min, &max, &step, &def, &caps));
                            check("IAMVideoProcAmp::Set", sub.am_video_proc_amp->Set(pu.property, def, VideoProcAmp_Flags_Manual));    
                        }
                    }
                    else check("IAMVideoProcAmp::Set", sub.am_video_proc_amp->Set(pu.property, value, VideoProcAmp_Flags_Manual));
                    return;
                }
            }
            throw std::runtime_error("unsupported control");
        }

        void get_pu_control_range(const device & device, int subdevice, rs_option option, int * min, int * max, int * step, int * def)
        {
            if(option >= RS_OPTION_COLOR_ENABLE_AUTO_EXPOSURE && option <= RS_OPTION_COLOR_ENABLE_AUTO_WHITE_BALANCE)
            {
                if(min)  *min  = 0;
                if(max)  *max  = 1;
                if(step) *step = 1;
                if(def)  *def  = 1;
                return;
            }

            auto & sub = device.subdevices[subdevice];
            const_cast<uvc::subdevice &>(sub).get_media_source();
            long minVal=0, maxVal=0, steppingDelta=0, defVal=0, capsFlag=0;
            if (option == RS_OPTION_COLOR_EXPOSURE)
            {
                check("IAMCameraControl::Get", sub.am_camera_control->GetRange(CameraControl_Exposure, &minVal, &maxVal, &steppingDelta, &defVal, &capsFlag));
                if (min)  *min = minVal;
                if (max)  *max = maxVal;
                if (step) *step = steppingDelta;
                if (def)  *def = defVal;
                return;
            }
            for(auto & pu : pu_controls)
            {
                if(option == pu.option)
                {
                    check("IAMVideoProcAmp::GetRange", sub.am_video_proc_amp->GetRange(pu.property, &minVal, &maxVal, &steppingDelta, &defVal, &capsFlag));
                    if(min)  *min  = static_cast<int>(minVal);
                    if(max)  *max  = static_cast<int>(maxVal);
                    if(step) *step = static_cast<int>(steppingDelta);
                    if(def)  *def  = static_cast<int>(defVal);
                    return;
                }
            }
            throw std::runtime_error("unsupported control");
        }

        void get_extension_control_range(const device & device, const extension_unit & xu, char control , int * min, int * max, int * step, int * def)
        {
            auto ks_control = const_cast<uvc::device &>(device).get_ks_control(xu);

            /* get step, min and max values*/
            KSP_NODE node;
            memset(&node, 0, sizeof(KSP_NODE));
            node.Property.Set = reinterpret_cast<const GUID &>(xu.id);
            node.Property.Id = control;
            node.Property.Flags = KSPROPERTY_TYPE_BASICSUPPORT | KSPROPERTY_TYPE_TOPOLOGY;
            node.NodeId = xu.node;

            KSPROPERTY_DESCRIPTION description;
            unsigned long bytes_received = 0;
            check("IKsControl::KsProperty", ks_control->KsProperty(
                (PKSPROPERTY)&node,
                sizeof(node),
                &description,
                sizeof(KSPROPERTY_DESCRIPTION),
                &bytes_received));

            unsigned long size = description.DescriptionSize;
            std::vector<BYTE> buffer((long)size);

            check("IKsControl::KsProperty", ks_control->KsProperty(
                (PKSPROPERTY)&node,
                sizeof(node),
                buffer.data(),
                size,
                &bytes_received));

            if (bytes_received != size) { throw  std::runtime_error("wrong data"); }

            BYTE * pRangeValues = buffer.data() + sizeof(KSPROPERTY_MEMBERSHEADER) + sizeof(KSPROPERTY_DESCRIPTION);

            *step = (int)*pRangeValues;
            pRangeValues++;
            *min = (int)*pRangeValues;
            pRangeValues++;
            *max = (int)*pRangeValues;


            /* get def value*/
            memset(&node, 0, sizeof(KSP_NODE));
            node.Property.Set = reinterpret_cast<const GUID &>(xu.id);
            node.Property.Id = control;
            node.Property.Flags = KSPROPERTY_TYPE_DEFAULTVALUES | KSPROPERTY_TYPE_TOPOLOGY;
            node.NodeId = xu.node;

            bytes_received = 0;
            check("IKsControl::KsProperty", ks_control->KsProperty(
                (PKSPROPERTY)&node,
                sizeof(node),
                &description,
                sizeof(KSPROPERTY_DESCRIPTION),
                &bytes_received));

            size = description.DescriptionSize;
            buffer.clear();
            buffer.resize(size);

            check("IKsControl::KsProperty", ks_control->KsProperty(
                (PKSPROPERTY)&node,
                sizeof(node),
                buffer.data(),
                size,
                &bytes_received));

            if (bytes_received != size) { throw  std::runtime_error("wrong data"); }

            pRangeValues = buffer.data() + sizeof(KSPROPERTY_MEMBERSHEADER) + sizeof(KSPROPERTY_DESCRIPTION);

            *def = (int)*pRangeValues;
        }

        int get_pu_control(const device & device, int subdevice, rs_option option)
        {
            auto & sub = device.subdevices[subdevice];
            // first call to get_media_source is also initializing the am_camera_control pointer, required for this method
            const_cast<uvc::subdevice &>(sub).get_media_source(); // initialize am_camera_control
            long value=0, flags=0;
            if (option == RS_OPTION_COLOR_EXPOSURE)
            {
                // am_camera_control != null, because get_media_source was called at least once
                check("IAMCameraControl::Get", sub.am_camera_control->Get(CameraControl_Exposure, &value, &flags));
                return value;
            }
            if(option == RS_OPTION_COLOR_ENABLE_AUTO_EXPOSURE)
            {
                check("IAMCameraControl::Get", sub.am_camera_control->Get(CameraControl_Exposure, &value, &flags));
                return flags == CameraControl_Flags_Auto;          
            }
            for(auto & pu : pu_controls)
            {
                if(option == pu.option)
                {
                    check("IAMVideoProcAmp::Get", sub.am_video_proc_amp->Get(pu.property, &value, &flags));
                    if(pu.enable_auto) return flags == VideoProcAmp_Flags_Auto;
                    else return value;
                }
            }
            throw std::runtime_error("unsupported control");
        }

        /////////////
        // context //
        /////////////

        std::shared_ptr<context> create_context()
        {
            return std::make_shared<context>();
        }

        bool is_device_connected(device & device, int vid, int pid)
        {
            for(auto& dev : device.subdevices)
            {
                if(dev.vid == vid && dev.pid == pid)
                    return true;
            }

            return false;
        }

        std::vector<std::shared_ptr<device>> query_devices(std::shared_ptr<context> context)
        {
            IMFAttributes * pAttributes = NULL;
            check("MFCreateAttributes", MFCreateAttributes(&pAttributes, 1));
            check("IMFAttributes::SetGUID", pAttributes->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID));
 
            IMFActivate ** ppDevices;
            UINT32 numDevices;
            check("MFEnumDeviceSources", MFEnumDeviceSources(pAttributes, &ppDevices, &numDevices));

            std::vector<std::shared_ptr<device>> devices;
            for(UINT32 i=0; i<numDevices; ++i)
            {
                com_ptr<IMFActivate> pDevice;
                *&pDevice = ppDevices[i];

                WCHAR * wchar_name = NULL; UINT32 length;
                pDevice->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK, &wchar_name, &length);
                auto name = win_to_utf(wchar_name);
                CoTaskMemFree(wchar_name);

                int vid, pid, mi; std::string unique_id;
                if(!parse_usb_path(vid, pid, mi, unique_id, name)) continue;

                std::shared_ptr<device> dev;
                for(auto & d : devices)
                {
                    if(d->vid == vid && d->pid == pid && d->unique_id == unique_id)
                    {
                        dev = d;
                    }
                }
                if(!dev)
                {
                    dev = std::make_shared<device>(context, vid, pid, unique_id);
                    devices.push_back(dev);
                }

                size_t subdevice_index = mi/2;
                if(subdevice_index >= dev->subdevices.size()) dev->subdevices.resize(subdevice_index+1);

                dev->subdevices[subdevice_index].reader_callback = new reader_callback(dev, static_cast<int>(subdevice_index));
                dev->subdevices[subdevice_index].mf_activate = pDevice;                
                dev->subdevices[subdevice_index].vid = vid;
                dev->subdevices[subdevice_index].pid = pid;
            }

            for(auto& devA : devices) // Look for CX3 Fisheye camera
            {
               if(devA->vid == VID_INTEL_CAMERA && devA->pid == ZR300_FISHEYE_PID)
               {
                    for(auto& devB : devices) // Look for DS ZR300 camera
                    {
                        if(devB->vid == VID_INTEL_CAMERA && devB->pid == ZR300_CX3_PID)
                        {
                            devB->subdevices.resize(4);
                            devB->subdevices[3].reader_callback = new reader_callback(devB, static_cast<int>(3));
                            devB->subdevices[3].mf_activate = devA->subdevices[0].mf_activate;
                            devB->subdevices[3].vid = devB->aux_vid = VID_INTEL_CAMERA;
                            devB->subdevices[3].pid = devB->aux_pid = ZR300_FISHEYE_PID;
                            devB->aux_unique_id = devA->unique_id;
                            devices.erase(std::remove(devices.begin(), devices.end(), devA), devices.end());
                            break;
                        }
                    }
                    break;
                }
            }

            CoTaskMemFree(ppDevices);
            return devices;
        }

        std::wstring getPath(HANDLE h, ULONG index)
        {
            // get name length
            USB_NODE_CONNECTION_NAME name;
            name.ConnectionIndex = index;
            if (!DeviceIoControl(h, IOCTL_USB_GET_NODE_CONNECTION_NAME, &name, sizeof(name), &name, sizeof(name), nullptr, nullptr))
            {
                return std::wstring(L"");
            }

            // alloc space
            if (name.ActualLength < sizeof(name)) return std::wstring(L"");
            auto alloc = std::malloc(name.ActualLength);
            auto pName = std::shared_ptr<USB_NODE_CONNECTION_NAME>(reinterpret_cast<USB_NODE_CONNECTION_NAME *>(alloc), std::free);

            // get name
            pName->ConnectionIndex = index;
            if (DeviceIoControl(h, IOCTL_USB_GET_NODE_CONNECTION_NAME, pName.get(), name.ActualLength, pName.get(), name.ActualLength, nullptr, nullptr))
            {
                return std::wstring(pName->NodeName);
            }

            return std::wstring(L"");
        }

        bool handleNode(const std::wstring & targetKey, HANDLE h, ULONG index)
        {
            USB_NODE_CONNECTION_DRIVERKEY_NAME key;
            key.ConnectionIndex = index;

            if (!DeviceIoControl(h, IOCTL_USB_GET_NODE_CONNECTION_DRIVERKEY_NAME, &key, sizeof(key), &key, sizeof(key), nullptr, nullptr))
            {
                return false;
            }

            if (key.ActualLength < sizeof(key)) return false;

            auto alloc = std::malloc(key.ActualLength);
            if (!alloc) throw std::bad_alloc();
            auto pKey = std::shared_ptr<USB_NODE_CONNECTION_DRIVERKEY_NAME>(reinterpret_cast<USB_NODE_CONNECTION_DRIVERKEY_NAME *>(alloc), std::free);

            pKey->ConnectionIndex = index;
            if (DeviceIoControl(h, IOCTL_USB_GET_NODE_CONNECTION_DRIVERKEY_NAME, pKey.get(), key.ActualLength, pKey.get(), key.ActualLength, nullptr, nullptr))
            {
                //std::wcout << pKey->DriverKeyName << std::endl;
                if (targetKey == pKey->DriverKeyName) {
                    return true;
                }
                else return false;
            }

            return false;
        }

        std::string handleHub(const std::wstring & targetKey, const std::wstring & path)
        {
            if (path == L"") return "";
            std::wstring fullPath = L"\\\\.\\" + path;

            HANDLE h = CreateFile(fullPath.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
            if (h == INVALID_HANDLE_VALUE) return "";
            auto h_gc = std::shared_ptr<void>(h, CloseHandle);

            USB_NODE_INFORMATION info;
            if (!DeviceIoControl(h, IOCTL_USB_GET_NODE_INFORMATION, &info, sizeof(info), &info, sizeof(info), nullptr, nullptr))
                return "";

            // for each port on the hub
            for (ULONG i = 1; i <= info.u.HubInformation.HubDescriptor.bNumberOfPorts; ++i)
            {
                // allocate something or other
                char buf[sizeof(USB_NODE_CONNECTION_INFORMATION_EX)] = { 0 };
                PUSB_NODE_CONNECTION_INFORMATION_EX pConInfo = (PUSB_NODE_CONNECTION_INFORMATION_EX)buf;

                // get info about port i
                pConInfo->ConnectionIndex = i;
                if (!DeviceIoControl(h, IOCTL_USB_GET_NODE_CONNECTION_INFORMATION_EX, pConInfo, sizeof(buf), pConInfo, sizeof(buf), nullptr, nullptr))
                {
                    continue;
                }

                // check if device is connected
                if (pConInfo->ConnectionStatus != DeviceConnected)
                {
                    continue; // almost assuredly silently. I think this flag gets set for any port without a device
                }

                // if connected, handle correctly, setting the location info if the device is found
                std::string ret = "";
                if (pConInfo->DeviceIsHub) ret = handleHub(targetKey, getPath(h, i));
                else
                {
                    if (handleNode(targetKey, h, i))
                    {
                        ret = win_to_utf(fullPath.c_str()) + " " + std::to_string(i);
                    }
                }
                if (ret != "") return ret;
            }

            return "";
        }

        std::string get_usb_port_id(const device & device) // Not implemented for Windows at this point
        {
            SP_DEVINFO_DATA devInfo = { sizeof(SP_DEVINFO_DATA) };
            static_assert(sizeof(guid) == sizeof(GUID), "struct packing error"); // not sure this is needed. maybe because the original function gets the guid object from outside?

            // build a device info represent all imaging devices.
            HDEVINFO device_info = SetupDiGetClassDevsEx((const GUID *)&GUID_DEVINTERFACE_IMAGE,
                nullptr, 
                nullptr, 
                DIGCF_PRESENT,
                nullptr,
                nullptr,
                nullptr);
            if (device_info == INVALID_HANDLE_VALUE) throw std::runtime_error("SetupDiGetClassDevs");
            auto di = std::shared_ptr<void>(device_info, SetupDiDestroyDeviceInfoList);

            // enumerate all imaging devices.
            for (int member_index = 0; ; ++member_index)
            {
                SP_DEVICE_INTERFACE_DATA interfaceData = { sizeof(SP_DEVICE_INTERFACE_DATA) };
                unsigned long buf_size = 0;

                if (SetupDiEnumDeviceInfo(device_info, member_index, &devInfo) == FALSE)
                {
                    if (GetLastError() == ERROR_NO_MORE_ITEMS) break; // stop when none left
                    continue; // silently ignore other errors
                }

                // get the device ID of current device.
                if (CM_Get_Device_ID_Size(&buf_size, devInfo.DevInst, 0) != CR_SUCCESS)
                {
                    LOG_ERROR("CM_Get_Device_ID_Size failed");
                    return "";
                }
                
                auto alloc = std::malloc(buf_size * sizeof(WCHAR) + sizeof(WCHAR));
                if (!alloc) throw std::bad_alloc();
                auto pInstID = std::shared_ptr<WCHAR>(reinterpret_cast<WCHAR *>(alloc), std::free);
                if (CM_Get_Device_ID(devInfo.DevInst, pInstID.get(), buf_size * sizeof(WCHAR) + sizeof(WCHAR), 0) != CR_SUCCESS) {
                    LOG_ERROR("CM_Get_Device_ID failed");
                    return "";
                }

                if (pInstID == nullptr) continue;

                // Check if this is our device 
                int usb_vid, usb_pid, usb_mi; std::string usb_unique_id;
                if (!parse_usb_path_from_device_id(usb_vid, usb_pid, usb_mi, usb_unique_id, std::string(win_to_utf(pInstID.get())))) continue;
                if (usb_vid != device.vid || usb_pid != device.pid || /* usb_mi != device->mi || */ usb_unique_id != device.unique_id) continue;

                // get parent (composite device) instance
                DEVINST instance;
                if (CM_Get_Parent(&instance, devInfo.DevInst, 0) != CR_SUCCESS)
                {
                    LOG_ERROR("CM_Get_Parent failed");
                    return "";
                }

                // get composite device instance id
                if (CM_Get_Device_ID_Size(&buf_size, instance, 0) != CR_SUCCESS)
                {
                    LOG_ERROR("CM_Get_Device_ID_Size failed");
                    return "";
                }
                alloc = std::malloc(buf_size*sizeof(WCHAR) + sizeof(WCHAR));
                if (!alloc) throw std::bad_alloc();
                pInstID = std::shared_ptr<WCHAR>(reinterpret_cast<WCHAR *>(alloc), std::free);
                if (CM_Get_Device_ID(instance, pInstID.get(), buf_size * sizeof(WCHAR) + sizeof(WCHAR), 0) != CR_SUCCESS) {
                    LOG_ERROR("CM_Get_Device_ID failed");
                    return "";
                }

                // upgrade to DEVINFO_DATA for SetupDiGetDeviceRegistryProperty
                device_info = SetupDiGetClassDevs(nullptr, pInstID.get(), nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE | DIGCF_ALLCLASSES);
                if (device_info == INVALID_HANDLE_VALUE) {
                    LOG_ERROR("SetupDiGetClassDevs failed");
                    return "";
                }
                auto di_gc = std::shared_ptr<void>(device_info, SetupDiDestroyDeviceInfoList);

                interfaceData = { sizeof(SP_DEVICE_INTERFACE_DATA) };
                if (SetupDiEnumDeviceInterfaces(device_info, nullptr, &GUID_DEVINTERFACE_USB_DEVICE, 0, &interfaceData) == FALSE)
                {
                    LOG_ERROR("SetupDiEnumDeviceInterfaces failed");
                    return "";
                }

                // get the SP_DEVICE_INTERFACE_DETAIL_DATA object, and also grab the SP_DEVINFO_DATA object for the device
                buf_size = 0;
                SetupDiGetDeviceInterfaceDetail(device_info, &interfaceData, nullptr, 0, &buf_size, nullptr);
                if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
                {
                    LOG_ERROR("SetupDiGetDeviceInterfaceDetail failed");
                    return "";
                }
                alloc = std::malloc(buf_size);
                if (!alloc) throw std::bad_alloc();
                auto detail_data = std::shared_ptr<SP_DEVICE_INTERFACE_DETAIL_DATA>(reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA *>(alloc), std::free);
                detail_data->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
                SP_DEVINFO_DATA parent_data = { sizeof(SP_DEVINFO_DATA) };
                if (!SetupDiGetDeviceInterfaceDetail(device_info, &interfaceData, detail_data.get(), buf_size, nullptr, &parent_data))
                {
                    LOG_ERROR("SetupDiGetDeviceInterfaceDetail failed");
                    return "";
                }

                // get driver key for composite device
                buf_size = 0;
                SetupDiGetDeviceRegistryProperty(device_info, &parent_data, SPDRP_DRIVER, nullptr, nullptr, 0, &buf_size);
                if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
                {
                    LOG_ERROR("SetupDiGetDeviceRegistryProperty failed in an unexpected manner");
                    return "";
                }
                alloc = std::malloc(buf_size);
                if (!alloc) throw std::bad_alloc();
                auto driver_key = std::shared_ptr<BYTE>(reinterpret_cast<BYTE*>(alloc), std::free);
                if (!SetupDiGetDeviceRegistryProperty(device_info, &parent_data, SPDRP_DRIVER, nullptr, driver_key.get(), buf_size, nullptr))
                {
                    LOG_ERROR("SetupDiGetDeviceRegistryProperty failed");
                    return "";
                }

                // contains composite device key
                std::wstring targetKey(reinterpret_cast<const wchar_t*>(driver_key.get()));

                // recursively check all hubs, searching for composite device
                std::wstringstream buf;
                for (int i = 0;; i++)
                { 
                    buf << "\\\\.\\HCD" << i;
                    std::wstring hcd = buf.str();

                    // grab handle
                    HANDLE h = CreateFile(hcd.c_str(), GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
                    auto h_gc = std::shared_ptr<void>(h, CloseHandle);
                    if (h == INVALID_HANDLE_VALUE)
                    {
                        LOG_ERROR("CreateFile failed");
                        break;
                    }
                    else
                    {
                        USB_ROOT_HUB_NAME name;

                        // get required space
                        if (!DeviceIoControl(h, IOCTL_USB_GET_ROOT_HUB_NAME, nullptr, 0, &name, sizeof(name), nullptr, nullptr)) {
                            LOG_ERROR("DeviceIoControl failed");
                            return ""; // alt: fail silently and hope its on a different root hub
                        }

                        // alloc space
                        alloc = std::malloc(name.ActualLength);
                        if (!alloc) throw std::bad_alloc();
                        auto pName = std::shared_ptr<USB_ROOT_HUB_NAME>(reinterpret_cast<USB_ROOT_HUB_NAME *>(alloc), std::free);

                        // get name
                        if (!DeviceIoControl(h, IOCTL_USB_GET_ROOT_HUB_NAME, nullptr, 0, pName.get(), name.ActualLength, nullptr, nullptr)) {
                            LOG_ERROR("DeviceIoControl failed");
                            return ""; // alt: fail silently and hope its on a different root hub
                        }

                        // return location if device is connected under this root hub
                        std::string ret = handleHub(targetKey, std::wstring(pName->RootHubName));
                        if (ret != "") return ret;
                    }
                }
            }
            throw std::exception("could not find camera in windows device tree");
        }
    }
}

#endif
