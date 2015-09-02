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
                if(desc) uvc_free_device_descriptor(desc);
                if(device) uvc_unref_device(device);
            }
        };

        struct device_handle::_impl
        {
            std::shared_ptr<device::_impl> parent;
            uvc_device_handle_t * handle;
            uvc_stream_ctrl_t ctrl;
            std::function<void(const void * frame, int width, int height, frame_format format)> callback;
            std::vector<int> claimed_interfaces;

            _impl(std::shared_ptr<device::_impl> parent, int subdevice_index) : handle()
            {
                this->parent = parent;
                check("uvc_open2", uvc_open2(parent->device, &handle, subdevice_index));
            }
            ~_impl()
            {
                for(auto interface_number : claimed_interfaces)
                {
                    int status = libusb_release_interface(handle->usb_devh, interface_number);
                    if(status < 0) std::cerr << "Warning: libusb_release_interface(...) returned " << libusb_error_name(status) << std::endl;
                }
                uvc_close(handle);
            }
        };

        ///////////////////
        // device_handle //
        ///////////////////

        void device_handle::get_stream_ctrl_format_size(int width, int height, frame_format cf, int fps)
        {
            check("get_stream_ctrl_format_size", uvc_get_stream_ctrl_format_size(impl->handle, &impl->ctrl, (uvc_frame_format)cf, width, height, fps));
        }

        void device_handle::start_streaming(std::function<void(const void * frame, int width, int height, frame_format format)> callback)
        {
            #if defined (ENABLE_DEBUG_SPAM)
            uvc_print_stream_ctrl(&impl->ctrl, stdout);
            #endif

            impl->callback = callback;
            check("uvc_start_streaming", uvc_start_streaming(impl->handle, &impl->ctrl, [](uvc_frame * frame, void * user)
            {
                reinterpret_cast<device_handle::_impl *>(user)->callback(frame->data, frame->width, frame->height, (frame_format)frame->frame_format);
            }, impl.get(), 0));
        }

        void device_handle::stop_streaming()
        {
            uvc_stop_streaming(impl->handle);
        }

        void device_handle::get_ctrl(uint8_t unit, uint8_t ctrl, void *data, int len)
        {
            int status = uvc_get_ctrl(impl->handle, unit, ctrl, data, len, UVC_GET_CUR);
            if(status < 0) throw std::runtime_error(to_string() << "uvc_get_ctrl(...) returned " << libusb_error_name(status));
        }

        void device_handle::set_ctrl(uint8_t unit, uint8_t ctrl, void *data, int len)
        {
            int status = uvc_set_ctrl(impl->handle, unit, ctrl, data, len);
            if(status < 0) throw std::runtime_error(to_string() << "uvc_set_ctrl(...) returned " << libusb_error_name(status));
        }

        void device_handle::claim_interface(int interface_number)
        {
            int status = libusb_claim_interface(impl->handle->usb_devh, interface_number);
            if(status < 0) throw std::runtime_error(to_string() << "libusb_claim_interface(...) returned " << libusb_error_name(status));
            impl->claimed_interfaces.push_back(interface_number);
        }

        void device_handle::bulk_transfer(unsigned char endpoint, unsigned char *data, int length, int *actual_length, unsigned int timeout)
        {
            int status = libusb_bulk_transfer(impl->handle->usb_devh, endpoint, data, length, actual_length, timeout);
            if(status < 0) throw std::runtime_error(to_string() << "libusb_bulk_transfer(...) returned " << libusb_error_name(status));
        }

        ////////////
        // device //
        ////////////

        int device::get_vendor_id() const { return impl->desc->idVendor; }
        int device::get_product_id() const { return impl->desc->idProduct; }
        const char * device::get_product_name() const { return impl->desc->product; }

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

#include <mfapi.h>			// For MFStartup, etc.
#include <mfidl.h>			// For MF_DEVSOURCE_*, etc.
#include <mfreadwrite.h>    // MFCreateSourceReaderFromMediaSource
#include <mferror.h>
#include <d3d9.h>

#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")


#include <uuids.h>
#include <vidcap.h>
#include <ksmedia.h>
#include <ksproxy.h>

#include <thread>
#include <chrono>
#include <algorithm>
#include <iostream>

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
			len = WideCharToMultiByte(CP_UTF8, 0, s, -1, &buffer[0], buffer.size()+1, NULL, NULL);
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

			void release()
			{
				if(p)
				{
					p->Release();
					p = nullptr;
				}
			}
		public:
			com_ptr() : p() {}
			com_ptr(nullptr_t) : p() {}
			com_ptr(const com_ptr & r) : p() { *this = r; }
			~com_ptr() { release(); }

			operator T * () const { return p; }
			T & operator * () const { return *p; }
			T * operator -> () const { return p; }

			com_ptr & operator = (const com_ptr & r)
			{ 
				if(this != &r)
				{
					release();
					p = r.p;
					if(p) p->AddRef();
				}
				return *this; 
			}

			T ** operator & ()
			{
				release();
				return &p;
			}
		};

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
            std::shared_ptr<context::_impl> parent;

			int vid, pid;
			std::string unique_id;
			std::vector<com_ptr<IMFActivate>> subdevices;

            _impl() : vid(), pid() {}
            _impl(std::shared_ptr<context::_impl> parent) : _impl()
            {
                this->parent = parent;
            }
        };

        struct device_handle::_impl
        {
            std::shared_ptr<device::_impl> parent;
			int subdevice_index;

			com_ptr<IMFMediaSource> pSource;
			com_ptr<IMFSourceReader> pReader;
			com_ptr<IKsControl> pControl;

            _impl(std::shared_ptr<device::_impl> parent, int subdevice_index) : subdevice_index(subdevice_index)
            {
                this->parent = parent;

				check("IMFActivate::ActivateObject", parent->subdevices[subdevice_index]->ActivateObject(__uuidof(IMFMediaSource), (void **)&pSource));

				com_ptr<IMFAttributes> pAttributes;
				check("MFCreateAttributes", MFCreateAttributes(&pAttributes, 0));
				// hr = pAttributes->SetUINT32(MF_READWRITE_DISABLE_CONVERTERS, TRUE);
				// hr = pAttributes->SetUnknown(MF_SOURCE_READER_ASYNC_CALLBACK, this);			
				check("MFCreateSourceReaderFromMediaSource", MFCreateSourceReaderFromMediaSource(pSource, pAttributes, &pReader));
            }

			void obtain_control_node()
			{
				if(pControl) return;
				
				IKsTopologyInfo * pKsTopologyInfo = NULL;
				check("QueryInterface", pSource->QueryInterface(__uuidof(pKsTopologyInfo), (void **)&pKsTopologyInfo));

				DWORD dwNumNodes = 0;
				check("get_numNodes", pKsTopologyInfo->get_NumNodes(&dwNumNodes));

				const GUID KSNODETYPE_DEV_SPECIFIC_LOCAL{0x941C7AC0L, 0xC559, 0x11D0, {0x8A, 0x2B, 0x00, 0xA0, 0xC9, 0x25, 0x5A, 0xC1}};

				GUID guidNodeType;
				check("get_nodeType", pKsTopologyInfo->get_NodeType(XUNODEID, &guidNodeType));

				if (guidNodeType == KSNODETYPE_DEV_SPECIFIC_LOCAL)
				{
					com_ptr<IUnknown> pUnknown;
					check("CreateNodeInstance", pKsTopologyInfo->CreateNodeInstance(XUNODEID, IID_IUnknown, (LPVOID *)&pUnknown));
					check("QueryInterface", pUnknown->QueryInterface(__uuidof(IKsControl), (void **)&pControl));
				}

			    if (!pControl) throw std::runtime_error("unable to obtain control node");
			}
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

        void device_handle::get_stream_ctrl_format_size(int width, int height, frame_format cf, int fps)
        {
			for (DWORD j = 0; ; j++)
			{
				com_ptr<IMFMediaType> pType;
				HRESULT hr = impl->pReader->GetNativeMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, j, &pType);
				if (hr == MF_E_NO_MORE_TYPES) break;
				check("IMFSourceReader::GetNativeMediaType", hr);

				UINT32 uvc_width, uvc_height, uvc_fps_num, uvc_fps_denom; GUID subtype;
				check("MFGetAttributeSize", MFGetAttributeSize(pType, MF_MT_FRAME_SIZE, &uvc_width, &uvc_height));
				if(uvc_width != width || uvc_height != height) continue;

				check("IMFMediaType::GetGUID", pType->GetGUID(MF_MT_SUBTYPE, &subtype));
				if(!matches_format(subtype, cf)) continue;

				check("MFGetAttributeRatio", MFGetAttributeRatio(pType, MF_MT_FRAME_RATE, &uvc_fps_num, &uvc_fps_denom));
				if(uvc_fps_denom == 0) continue;
				int uvc_fps = uvc_fps_num / uvc_fps_denom;
				if(std::abs(fps - uvc_fps) > 1) continue;

				check("IMFSourceReader::SetCurrentMediaType", impl->pReader->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, NULL, pType));
				std::cout << "Selected mode " << uvc_width << "x" << uvc_height << ":" << std::string((const char *)&subtype.Data1, ((const char *)&subtype.Data1)+4) << "@" << uvc_fps << " for stream" << std::endl;
				return;
			}
			throw std::runtime_error("no matching media type");
        }

        void device_handle::start_streaming(std::function<void(const void * frame, int width, int height, frame_format format)> callback)
        {
			// pSource->Start(...);
			// pReader->ReadSample(...);
			throw std::runtime_error("device_handle::start_streaming(...) not implemented");
        }

        void device_handle::stop_streaming()
        {
			throw std::runtime_error("device_handle::stop_streaming(...) not implemented");
        }

        void device_handle::get_ctrl(uint8_t unit, uint8_t ctrl, void *data, int len)
        {
			impl->obtain_control_node();

			KSP_NODE node;
			memset(&node, 0, sizeof(KSP_NODE));
			node.Property.Set = {0x18682d34, 0xdd2c, 0x4073, {0xad, 0x23, 0x72, 0x14, 0x73, 0x9a, 0x07, 0x4c}}; // GUID_EXTENSION_UNIT_DESCRIPTOR
			node.Property.Id = ctrl;
			node.Property.Flags = KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_TOPOLOGY;
			node.NodeId = XUNODEID;

			ULONG bytes_received = 0;
        	check("IKsControl::KsProperty", impl->pControl->KsProperty((PKSPROPERTY)&node, sizeof(node), data, len, &bytes_received));
			if(bytes_received != len) throw std::runtime_error("XU read did not return enough data");
        }

        void device_handle::set_ctrl(uint8_t unit, uint8_t ctrl, void *data, int len)
        {
			impl->obtain_control_node();
				
			KSP_NODE kspNode;
			memset(&kspNode, 0, sizeof(KSP_NODE));
			kspNode.Property.Set = {0x18682d34, 0xdd2c, 0x4073, {0xad, 0x23, 0x72, 0x14, 0x73, 0x9a, 0x07, 0x4c}}; // GUID_EXTENSION_UNIT_DESCRIPTOR
			kspNode.Property.Id = ctrl;
			kspNode.Property.Flags = KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_TOPOLOGY;
			kspNode.NodeId = XUNODEID;
				
			check("IKsControl::KsProperty", impl->pControl->KsProperty((PKSPROPERTY)&kspNode, sizeof(KSP_NODE), data, len, NULL));
        }

        void device_handle::claim_interface(int interface_number)
        {
			throw std::runtime_error("device_handle::claim_interface(...) not implemented");
        }

        void device_handle::bulk_transfer(unsigned char endpoint, unsigned char *data, int length, int *actual_length, unsigned int timeout)
        {
			throw std::runtime_error("device_handle::bulk_transfer(...) not implemented");
        }

        ////////////
        // device //
        ////////////

		int device::get_vendor_id() const { return impl->vid; }
		int device::get_product_id() const { return impl->pid; }
		const char * device::get_product_name() const { return ""; }

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
					dev = {std::make_shared<device::_impl>()};
					dev.impl->vid = vid;
					dev.impl->pid = pid;
					dev.impl->unique_id = unique_id;
					devices.push_back(dev);
				}

				int subdevice_index = mi/2;
				if(subdevice_index >= dev.impl->subdevices.size()) dev.impl->subdevices.resize(subdevice_index+1);
				dev.impl->subdevices[subdevice_index] = pDevice;
			}
			CoTaskMemFree(ppDevices);
			return devices;
		}
    }
}
#endif