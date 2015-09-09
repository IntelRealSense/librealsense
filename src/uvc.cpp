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

        struct subdevice
        {
            uvc_device_handle_t * handle = nullptr;
            uvc_stream_ctrl_t ctrl;
            std::function<void(const void * frame)> callback;
        };

        struct device::_impl
        {
            std::shared_ptr<context::_impl> parent;
            uvc_device_t * device;
            uvc_device_descriptor_t * desc;
            std::vector<subdevice> subdevices;
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
                    int status = libusb_release_interface(get_subdevice(0).handle->usb_devh, interface_number);
                    if(status < 0) std::cerr << "Warning: libusb_release_interface(...) returned " << libusb_error_name(status) << std::endl;
                }

                for(auto & sub : subdevices) if(sub.handle) uvc_close(sub.handle);
                if(desc) uvc_free_device_descriptor(desc);
                if(device) uvc_unref_device(device);
            }

            subdevice & get_subdevice(int subdevice_index)
            {
                if(subdevice_index >= subdevices.size()) subdevices.resize(subdevice_index+1);
                if(!subdevices[subdevice_index].handle) check("uvc_open2", uvc_open2(device, &subdevices[subdevice_index].handle, subdevice_index));
                return subdevices[subdevice_index];
            }
        };

        ////////////
        // device //
        ////////////

        int device::get_vendor_id() const { return impl->desc->idVendor; }
        int device::get_product_id() const { return impl->desc->idProduct; }

        void device::get_control(uint8_t unit, uint8_t ctrl, void * data, int len) const
        {
            int status = uvc_get_ctrl(impl->get_subdevice(0).handle, unit, ctrl, data, len, UVC_GET_CUR);
            if(status < 0) throw std::runtime_error(to_string() << "uvc_get_ctrl(...) returned " << libusb_error_name(status));
        }

        void device::set_control(uint8_t unit, uint8_t ctrl, void * data, int len)
        {
            int status = uvc_set_ctrl(impl->get_subdevice(0).handle, unit, ctrl, data, len);
            if(status < 0) throw std::runtime_error(to_string() << "uvc_set_ctrl(...) returned " << libusb_error_name(status));
        }

        void device::claim_interface(const guid & interface_guid, int interface_number)
        {
            int status = libusb_claim_interface(impl->get_subdevice(0).handle->usb_devh, interface_number);
            if(status < 0) throw std::runtime_error(to_string() << "libusb_claim_interface(...) returned " << libusb_error_name(status));
            impl->claimed_interfaces.push_back(interface_number);
        }

        void device::bulk_transfer(unsigned char endpoint, void * data, int length, int *actual_length, unsigned int timeout)
        {
            int status = libusb_bulk_transfer(impl->get_subdevice(0).handle->usb_devh, endpoint, (unsigned char *)data, length, actual_length, timeout);
            if(status < 0) throw std::runtime_error(to_string() << "libusb_bulk_transfer(...) returned " << libusb_error_name(status));
        }

        void device::set_subdevice_mode(int subdevice_index, int width, int height, frame_format cf, int fps, std::function<void(const void * frame)> callback)
        {
            auto & sub = impl->get_subdevice(subdevice_index);
            check("get_stream_ctrl_format_size", uvc_get_stream_ctrl_format_size(sub.handle, &sub.ctrl, (uvc_frame_format)cf, width, height, fps));
            sub.callback = callback;
        }

        void device::start_streaming()
        {
            for(auto & sub : impl->subdevices)
            {
                if(sub.handle)
                {
                    #if defined (ENABLE_DEBUG_SPAM)
                    uvc_print_stream_ctrl(&sub.ctrl, stdout);
                    #endif

                    check("uvc_start_streaming", uvc_start_streaming(sub.handle, &sub.ctrl, [](uvc_frame * frame, void * user)
                    {
                        reinterpret_cast<subdevice *>(user)->callback(frame->data);
                    }, &sub, 0));
                }
            }
        }

        void device::stop_streaming()
        {
            for(auto & sub : impl->subdevices)
            {
                if(sub.handle) uvc_stop_streaming(sub.handle);
            }
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

#include <Cfgmgr32.h>
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
                std::cerr << "malformed usb device path: " << name << std::endl;
                return false;
            }

            auto ids = tokenize(tokens[1], '&');
            if(ids[0].size() != 8 || ids[0].substr(0,4) != "vid_" || !(std::istringstream(ids[0].substr(4,4)) >> std::hex >> vid))
            {
                std::cerr << "malformed vid string: " << tokens[1] << std::endl;
                return false;
            }

            if(ids[1].size() != 8 || ids[1].substr(0,4) != "pid_" || !(std::istringstream(ids[1].substr(4,4)) >> std::hex >> pid))
            {
                std::cerr << "malformed pid string: " << tokens[1] << std::endl;
                return false;
            }

            if(ids[2].size() != 5 || ids[2].substr(0,3) != "mi_" || !(std::istringstream(ids[2].substr(3,2)) >> mi))
            {
                std::cerr << "malformed mi string: " << tokens[1] << std::endl;
                return false;
            }

            ids = tokenize(tokens[2], '&');
            if(ids.size() < 2)
            {
                std::cerr << "malformed id string: " << tokens[2] << std::endl;
                return false;
            }
            unique_id = ids[1];
            return true;
        }

        struct context::_impl
        {
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

        class reader_callback : public IMFSourceReaderCallback
        {
            std::weak_ptr<device::_impl> owner; // The device holds a reference to us, so use weak_ptr to prevent a cycle
            int subdevice_index;
            ULONG ref_count;
            volatile bool streaming = false;
        public:
            reader_callback(std::weak_ptr<device::_impl> owner, int subdevice_index) : owner(owner), subdevice_index(subdevice_index), ref_count() {}

            bool is_streaming() const { return streaming; }
            void on_start() { streaming = true; }

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
            HRESULT STDMETHODCALLTYPE OnReadSample(HRESULT hrStatus, DWORD dwStreamIndex, DWORD dwStreamFlags, LONGLONG llTimestamp, IMFSample * sample) override;
            HRESULT STDMETHODCALLTYPE OnFlush(DWORD dwStreamIndex) override { streaming = false; return S_OK; }
            HRESULT STDMETHODCALLTYPE OnEvent(DWORD dwStreamIndex, IMFMediaEvent *pEvent) override { return S_OK; }
        };

        struct subdevice
        {
            com_ptr<reader_callback> reader_callback;
            com_ptr<IMFActivate> mf_activate;
            com_ptr<IMFMediaSource> mf_media_source;
            com_ptr<IMFSourceReader> mf_source_reader;
            std::function<void(const void * frame)> callback;

            com_ptr<IMFMediaSource> get_media_source()
            {
                if(!mf_media_source) check("IMFActivate::ActivateObject", mf_activate->ActivateObject(__uuidof(IMFMediaSource), (void **)&mf_media_source));
                return mf_media_source;
            }
        };

        struct device::_impl
        {
            const std::shared_ptr<context::_impl> parent;
            const int vid, pid;
            const std::string unique_id;

            std::vector<subdevice> subdevices;
            com_ptr<IKsControl> ks_control;

			HANDLE usb_file_handle = INVALID_HANDLE_VALUE;
			WINUSB_INTERFACE_HANDLE usb_interface_handle = INVALID_HANDLE_VALUE;

            _impl(std::shared_ptr<context::_impl> parent, int vid, int pid, std::string unique_id) : parent(move(parent)), vid(vid), pid(pid), unique_id(move(unique_id)) {}
            ~_impl() { stop_streaming(); close_win_usb(); }

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
            }

            com_ptr<IMFMediaSource> get_media_source(int subdevice_index)
            {
                return subdevices[subdevice_index].get_media_source();
            }			

			void open_win_usb(const guid & interface_guid, int interface_number) try
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
                        std::cerr << "SetupDiGetDeviceInterfaceDetail failed" << std::endl;
                        continue;
                    }
                    auto alloc = std::malloc(detail_data_size);
                    if(!alloc) throw std::bad_alloc();

                    // Retrieve the detail data struct
                    auto detail_data = std::shared_ptr<SP_DEVICE_INTERFACE_DETAIL_DATA>(reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA *>(alloc), std::free);
                    detail_data->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
				    if (!SetupDiGetDeviceInterfaceDetail(device_info, &interfaceData, detail_data.get(), detail_data_size, nullptr, nullptr))
				    {
					    std::cerr << "SetupDiGetDeviceInterfaceDetail failed" << std::endl;
					    continue;
				    }
                    if (detail_data->DevicePath == nullptr) continue;

                    // Check if this is our device
                    int usb_vid, usb_pid, usb_mi; std::string usb_unique_id;
                    if(!parse_usb_path(usb_vid, usb_pid, usb_mi, usb_unique_id, win_to_utf(detail_data->DevicePath))) continue;
                    if(usb_vid != vid || usb_pid != pid || usb_mi != interface_number || usb_unique_id != unique_id) continue;                    
                        
					usb_file_handle = CreateFile(detail_data->DevicePath, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_WRITE | FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, nullptr);
					if (usb_file_handle == INVALID_HANDLE_VALUE) throw std::runtime_error("CreateFile(...) failed");

				    if(!WinUsb_Initialize(usb_file_handle, &usb_interface_handle))
                    {
					    std::cerr << "Last Error: " << GetLastError() << std::endl;
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
					std::cerr << "WinUsb_ReadPipe failure... lastError: " << lastError << std::endl;
					result = false;
				}

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
                            owner_ptr->subdevices[subdevice_index].callback(byte_buffer);
                            HRESULT hr = buffer->Unlock();
                        }
                    }
                }

                HRESULT hr = owner_ptr->subdevices[subdevice_index].mf_source_reader->ReadSample(MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, NULL, NULL, NULL, NULL);
                switch(hr)
                {
                case S_OK: break;
                case MF_E_INVALIDREQUEST: std::cout << "ReadSample returned MF_E_INVALIDREQUEST" << std::endl; break;
                case MF_E_INVALIDSTREAMNUMBER: std::cout << "ReadSample returned MF_E_INVALIDSTREAMNUMBER" << std::endl; break;
                case MF_E_NOTACCEPTING: std::cout << "ReadSample returned MF_E_NOTACCEPTING" << std::endl; break;
                case E_INVALIDARG: std::cout << "ReadSample returned E_INVALIDARG" << std::endl; break;
                case MF_E_VIDEO_RECORDING_DEVICE_INVALIDATED: std::cout << "ReadSample returned MF_E_VIDEO_RECORDING_DEVICE_INVALIDATED" << std::endl; break;
                default: std::cout << "ReadSample returned HRESULT " << std::hex << (uint32_t)hr << std::endl; break;
                }
            }
            return S_OK; 
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

        void device::claim_interface(const guid & interface_guid, int interface_number)
        {
			impl->open_win_usb(interface_guid, interface_number);
        }

        void device::bulk_transfer(uint8_t endpoint, void * data, int length, int *actual_length, unsigned int timeout)
        {		
            if(USB_ENDPOINT_DIRECTION_OUT(endpoint))
            {
            	impl->usb_synchronous_write(endpoint, data, length, timeout);
            }
            
            if(USB_ENDPOINT_DIRECTION_IN(endpoint))
			{
				auto actualLen = ULONG(actual_length);
				impl->usb_synchronous_read(endpoint, data, length, actual_length, timeout);
			}
        }

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

        void device::set_subdevice_mode(int subdevice_index, int width, int height, frame_format cf, int fps, std::function<void(const void * frame)> callback)
        {
            auto & sub = impl->subdevices[subdevice_index];
            
            if(!sub.mf_source_reader)
            {
                com_ptr<IMFAttributes> pAttributes;
                check("MFCreateAttributes", MFCreateAttributes(&pAttributes, 1));
                check("IMFAttributes::SetUnknown", pAttributes->SetUnknown(MF_SOURCE_READER_ASYNC_CALLBACK, static_cast<IUnknown *>(sub.reader_callback)));
                check("MFCreateSourceReaderFromMediaSource", MFCreateSourceReaderFromMediaSource(sub.get_media_source(), pAttributes, &sub.mf_source_reader));
            }

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
                if(!matches_format(subtype, cf)) continue;

                check("MFGetAttributeRatio", MFGetAttributeRatio(media_type, MF_MT_FRAME_RATE, &uvc_fps_num, &uvc_fps_denom));
                if(uvc_fps_denom == 0) continue;
                int uvc_fps = uvc_fps_num / uvc_fps_denom;
                if(std::abs(fps - uvc_fps) > 1) continue;

                check("IMFSourceReader::SetCurrentMediaType", sub.mf_source_reader->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, NULL, media_type));
                sub.callback = callback;
                return;
            }
            throw std::runtime_error("no matching media type");
        }

        void device::start_streaming() { impl->start_streaming(); }
        void device::stop_streaming() { impl->stop_streaming(); }

        /////////////
        // context //
        /////////////

        context context::create()
        {
            return {std::make_shared<context::_impl>()};
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

                WCHAR * wchar_name = NULL; UINT32 length;
                pDevice->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK, &wchar_name, &length);
                auto name = win_to_utf(wchar_name);
                CoTaskMemFree(wchar_name);

                int vid, pid, mi; std::string unique_id;
                if(!parse_usb_path(vid, pid, mi, unique_id, name)) continue;

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

                dev.impl->subdevices[subdevice_index].reader_callback = new reader_callback(dev.impl, subdevice_index);
                dev.impl->subdevices[subdevice_index].mf_activate = pDevice;                
            }
            CoTaskMemFree(ppDevices);
            return devices;
        }
    }
}
#endif
