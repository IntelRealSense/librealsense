// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#ifdef RS2_USE_WMF_BACKEND

#if (_MSC_FULL_VER < 180031101)
#error At least Visual Studio 2013 Update 4 is required to compile this backend
#endif

#define NOMINMAX

#include "win-uvc.h"
#include "../types.h"

#include "Shlwapi.h"
#include <Windows.h>
#include "mfapi.h"
#include <vidcap.h>
#include <Mferror.h>

#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")

#define type_guid  MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID
#define did_guid  MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK

#define DEVICE_NOT_READY_ERROR _HRESULT_TYPEDEF_(0x80070015L)

namespace rsimpl2
{
    namespace uvc
    {
        // we are using standard fourcc codes to represent formats, while MF is using GUIDs
        // occasionally there is a mismatch between the code and the guid data
        const std::unordered_map<uint32_t, uint32_t> fourcc_map = {
            { 0x59382020, 0x47524559 },    /* 'GREY' from 'Y8  ' */
            { 0x52573130, 0x70524141 },    /* 'pRAA' from 'RW10'.*/
            { 0x32000000, 0x47524559 },    /* 'GREY' from 'L8  '   */
            { 0x50000000, 0x5a313620 },    /* 'Z16'  from 'D16 '    */
            { 0x52415738, 0x47524559 }     /* 'GREY' from 'RAW8 '    */
        };

        STDMETHODIMP source_reader_callback::QueryInterface(REFIID iid, void** ppv)
        {
#pragma warning( push )
#pragma warning(disable : 4838)
            static const QITAB qit[] =
            {
                QITABENT(source_reader_callback, IMFSourceReaderCallback),
                { nullptr },
            };
            return QISearch(this, qit, iid, ppv);
#pragma warning( pop )
        };

        STDMETHODIMP_(ULONG) source_reader_callback::AddRef() { return InterlockedIncrement(&_refCount); }

        STDMETHODIMP_(ULONG) source_reader_callback::Release()  {
            ULONG count = InterlockedDecrement(&_refCount);
            if (count <= 0)
            {
                delete this;
            }
            return count;
        }

        STDMETHODIMP source_reader_callback::OnReadSample(HRESULT hrStatus,
            DWORD dwStreamIndex,
            DWORD /*dwStreamFlags*/,
            LONGLONG /*llTimestamp*/,
            IMFSample *sample)
        {
            auto owner = _owner.lock();
            if (owner && owner->_reader)
            {
                if (FAILED(hrStatus)) owner->_readsample_result = hrStatus;
                owner->_has_started.set();

                LOG_HR(owner->_reader->ReadSample(dwStreamIndex, 0, nullptr, nullptr, nullptr, nullptr));

                if (!owner->_is_started)
                    return S_OK;

                if (sample)
                {
                    CComPtr<IMFMediaBuffer> buffer = nullptr;
                    if (SUCCEEDED(sample->GetBufferByIndex(0, &buffer)))
                    {
                        byte* byte_buffer=nullptr;
                        DWORD max_length{}, current_length{};
                        if (SUCCEEDED(buffer->Lock(&byte_buffer, &max_length, &current_length)))
                        {
                            try
                            {
                                auto& stream = owner->_streams[dwStreamIndex];
                                std::lock_guard<std::mutex> lock(owner->_streams_mutex);
                                auto profile = stream.profile;
                                frame_object f{ current_length, 0, byte_buffer, nullptr };

                                auto continuation = [buffer, this]()
                                {
                                    buffer->Unlock();
                                };

                                stream.callback(profile, f, continuation);
                            }
                            catch (...)
                            {
                                // TODO: log
                            }
                        }
                    }
                }
            }

            return S_OK;
        };
        STDMETHODIMP source_reader_callback::OnEvent(DWORD /*sidx*/, IMFMediaEvent* /*event*/) { return S_OK; }
        STDMETHODIMP source_reader_callback::OnFlush(DWORD)
        {
            auto owner = _owner.lock();
            if (owner)
            {
                owner->_is_flushed.set();
            }
            return S_OK;
        }






        bool wmf_uvc_device::is_connected(const uvc_device_info& info)
        {
            auto result = false;
            foreach_uvc_device([&result, &info](const uvc_device_info& i, IMFActivate*)
            {
                if (i == info) result = true;
            });
            return result;
        }

        IKsControl* wmf_uvc_device::get_ks_control(const extension_unit & xu) const
        {
            auto it = _ks_controls.find(xu.node);
            if (it != std::end(_ks_controls)) return it->second;
            throw std::runtime_error("Extension control must be initialized before use!");
        }

        void wmf_uvc_device::init_xu(const extension_unit& xu)
        {
            if (!_source)
                throw std::runtime_error("Could not initialize extensions controls!");

            // Attempt to retrieve IKsControl
            CComPtr<IKsTopologyInfo> ks_topology_info = nullptr;
            CHECK_HR(_source->QueryInterface(__uuidof(IKsTopologyInfo),
                reinterpret_cast<void **>(&ks_topology_info)));

            DWORD nNodes=0;
            check("get_NumNodes", ks_topology_info->get_NumNodes(&nNodes));

            CComPtr<IUnknown> unknown = nullptr;
            CHECK_HR(ks_topology_info->CreateNodeInstance(xu.node, IID_IUnknown,
                reinterpret_cast<LPVOID *>(&unknown)));

            CComPtr<IKsControl> ks_control = nullptr;
            CHECK_HR(unknown->QueryInterface(__uuidof(IKsControl),
                reinterpret_cast<void **>(&ks_control)));
            _ks_controls[xu.node] = ks_control;
        }

        bool wmf_uvc_device::set_xu(const extension_unit& xu, uint8_t ctrl, const uint8_t* data, int len)
        {
            auto ks_control = get_ks_control(xu);

            KSP_NODE node;
            memset(&node, 0, sizeof(KSP_NODE));
            node.Property.Set = reinterpret_cast<const GUID &>(xu.id);
            node.Property.Id = ctrl;
            node.Property.Flags = KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_TOPOLOGY;
            node.NodeId = xu.node;

            ULONG bytes_received = 0;
            auto hr = ks_control->KsProperty(reinterpret_cast<PKSPROPERTY>(&node),
                sizeof(KSP_NODE), (void*)data, len, &bytes_received);

            if (hr == DEVICE_NOT_READY_ERROR)
                return false;

            CHECK_HR(hr);
            return true;
        }

        bool wmf_uvc_device::get_xu(const extension_unit& xu, uint8_t ctrl, uint8_t* data, int len) const
        {
            auto ks_control = get_ks_control(xu);

            KSP_NODE node;
            memset(&node, 0, sizeof(KSP_NODE));
            node.Property.Set = reinterpret_cast<const GUID &>(xu.id);
            node.Property.Id = ctrl;
            node.Property.Flags = KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_TOPOLOGY;
            node.NodeId = xu.node;

            ULONG bytes_received = 0;
            auto hr = ks_control->KsProperty(reinterpret_cast<PKSPROPERTY>(&node),
                sizeof(node), data, len, &bytes_received);

            if (hr == DEVICE_NOT_READY_ERROR)
                return false;

            if (bytes_received != len)
                throw std::runtime_error("XU read did not return enough data");

            CHECK_HR(hr);
            return true;
        }

        void ReadFromBuffer(control_range& cfg, BYTE* buffer, int length)
        {
            BYTE* next_struct = buffer;

            PKSPROPERTY_DESCRIPTION pDesc = reinterpret_cast<PKSPROPERTY_DESCRIPTION>(next_struct);
            next_struct += sizeof(KSPROPERTY_DESCRIPTION);

            if (pDesc->MembersListCount < 1)
                throw std::exception("no data ksprop");

            PKSPROPERTY_MEMBERSHEADER pHeader = reinterpret_cast<PKSPROPERTY_MEMBERSHEADER>(next_struct);
            next_struct += sizeof(KSPROPERTY_MEMBERSHEADER);

            if (pHeader->MembersCount < 1)
                throw std::exception("no data ksprop");

            // The data fields are up to four bytes
            auto field_width = std::min(sizeof(uint32_t), (size_t)length);
            auto option_range_size = std::max(sizeof(uint32_t), (size_t)length);
            switch (pHeader->MembersFlags)
            {
                /* member flag is not set correctly in current IvCam Implementation */
            case KSPROPERTY_MEMBER_RANGES:
            case KSPROPERTY_MEMBER_STEPPEDRANGES:
            {
                if (pDesc->DescriptionSize < sizeof(KSPROPERTY_DESCRIPTION) + sizeof(KSPROPERTY_MEMBERSHEADER) + 3 * sizeof(UCHAR))
                {
                    throw std::exception("no data ksprop");
                }

                auto pStruct = next_struct;
                cfg.step.resize(option_range_size);
                rsimpl2::copy(cfg.step.data(), pStruct, field_width);
                pStruct += length;
                cfg.min.resize(option_range_size);
                rsimpl2::copy(cfg.min.data(), pStruct, field_width);
                pStruct += length;
                cfg.max.resize(option_range_size);
                rsimpl2::copy(cfg.max.data(), pStruct, field_width);
                return;
            }
            case KSPROPERTY_MEMBER_VALUES:
            {
                /*
                *   we don't yet support reading a list of values, only min-max.
                *   so we only support reading default value from a list
                */

                if (pHeader->Flags == KSPROPERTY_MEMBER_FLAG_DEFAULT && pHeader->MembersCount == 1)
                {
                    if (pDesc->DescriptionSize < sizeof(KSPROPERTY_DESCRIPTION) + sizeof(KSPROPERTY_MEMBERSHEADER) + sizeof(UCHAR))
                    {
                        throw std::exception("no data ksprop");
                    }

                    cfg.def.resize(option_range_size);
                    rsimpl2::copy(cfg.def.data(), next_struct, field_width);
                }
                return;
            }
            default:
                throw  std::exception("unsupported");
            }
        }

        control_range wmf_uvc_device::get_xu_range(const extension_unit& xu, uint8_t ctrl, int len) const
        {
            auto ks_control = get_ks_control(xu);

            /* get step, min and max values*/
            KSP_NODE node;
            memset(&node, 0, sizeof(KSP_NODE));
            node.Property.Set = reinterpret_cast<const GUID &>(xu.id);
            node.Property.Id = ctrl;
            node.Property.Flags = KSPROPERTY_TYPE_BASICSUPPORT | KSPROPERTY_TYPE_TOPOLOGY;
            node.NodeId = xu.node;

            KSPROPERTY_DESCRIPTION description;
            unsigned long bytes_received = 0;
            CHECK_HR(ks_control->KsProperty(
                reinterpret_cast<PKSPROPERTY>(&node),
                sizeof(node),
                &description,
                sizeof(KSPROPERTY_DESCRIPTION),
                &bytes_received));

            auto size = description.DescriptionSize;
            std::vector<BYTE> buffer(static_cast<long>(size));

            CHECK_HR(ks_control->KsProperty(
                reinterpret_cast<PKSPROPERTY>(&node),
                sizeof(node),
                buffer.data(),
                size,
                &bytes_received));

            if (bytes_received != size) { throw  std::runtime_error("wrong data"); }

            control_range result{};
            ReadFromBuffer(result, buffer.data(), len);

            /* get def value*/
            memset(&node, 0, sizeof(KSP_NODE));
            node.Property.Set = reinterpret_cast<const GUID &>(xu.id);
            node.Property.Id = ctrl;
            node.Property.Flags = KSPROPERTY_TYPE_DEFAULTVALUES | KSPROPERTY_TYPE_TOPOLOGY;
            node.NodeId = xu.node;

            bytes_received = 0;
            CHECK_HR(ks_control->KsProperty(
                reinterpret_cast<PKSPROPERTY>(&node),
                sizeof(node),
                &description,
                sizeof(KSPROPERTY_DESCRIPTION),
                &bytes_received));

            size = description.DescriptionSize;
            buffer.clear();
            buffer.resize(size);

            CHECK_HR(ks_control->KsProperty(
                reinterpret_cast<PKSPROPERTY>(&node),
                sizeof(node),
                buffer.data(),
                size,
                &bytes_received));

            if (bytes_received != size) { throw  std::runtime_error("wrong data"); }

            ReadFromBuffer(result, buffer.data(), len);

            return result;
        }

        struct pu_control { rs2_option option; long property; bool enable_auto; };
        static const pu_control pu_controls[] = {
            { RS2_OPTION_BACKLIGHT_COMPENSATION, VideoProcAmp_BacklightCompensation },
            { RS2_OPTION_BRIGHTNESS, VideoProcAmp_Brightness },
            { RS2_OPTION_CONTRAST, VideoProcAmp_Contrast },
            { RS2_OPTION_GAIN, VideoProcAmp_Gain },
            { RS2_OPTION_GAMMA, VideoProcAmp_Gamma },
            { RS2_OPTION_HUE, VideoProcAmp_Hue },
            { RS2_OPTION_SATURATION, VideoProcAmp_Saturation },
            { RS2_OPTION_SHARPNESS, VideoProcAmp_Sharpness },
            { RS2_OPTION_WHITE_BALANCE, VideoProcAmp_WhiteBalance },
            { RS2_OPTION_ENABLE_AUTO_WHITE_BALANCE, VideoProcAmp_WhiteBalance, true },
        };

        bool wmf_uvc_device::get_pu(rs2_option opt, int32_t& value) const
        {
            long val = 0, flags = 0;
            if (opt == RS2_OPTION_EXPOSURE)
            {
                auto hr = _camera_control->Get(CameraControl_Exposure, &val, &flags);
                if (hr == DEVICE_NOT_READY_ERROR)
                    return false;

                value = val;
                CHECK_HR(hr);
                return true;
            }
            if (opt == RS2_OPTION_ENABLE_AUTO_EXPOSURE)
            {
                auto hr = _camera_control->Get(CameraControl_Exposure, &val, &flags);
                if (hr == DEVICE_NOT_READY_ERROR)
                    return false;

                value = (flags == CameraControl_Flags_Auto);
                CHECK_HR(hr);
                return true;
            }
            for (auto & pu : pu_controls)
            {
                if (opt == pu.option)
                {
                    auto hr = _video_proc->Get(pu.property, &val, &flags);
                    if (hr == DEVICE_NOT_READY_ERROR)
                        return false;

                    if (pu.enable_auto)
                        value = (flags == VideoProcAmp_Flags_Auto);
                    else
                        value = val;

                    CHECK_HR(hr);
                    return true;
                }
            }
            throw std::runtime_error("Unsupported control!");
        }

        bool wmf_uvc_device::set_pu(rs2_option opt, int value)
        {
            if (opt == RS2_OPTION_EXPOSURE)
            {
                auto hr = _camera_control->Set(CameraControl_Exposure, static_cast<int>(value), CameraControl_Flags_Manual);
                if (hr == DEVICE_NOT_READY_ERROR)
                    return false;

                CHECK_HR(hr);
                return true;
            }
            if (opt == RS2_OPTION_ENABLE_AUTO_EXPOSURE)
            {
                if (value)
                {
                    auto hr = _camera_control->Set(CameraControl_Exposure, 0, CameraControl_Flags_Auto);
                    if (hr == DEVICE_NOT_READY_ERROR)
                        return false;

                    CHECK_HR(hr);
                }
                else
                {
                    long min, max, step, def, caps;
                    auto hr = _camera_control->GetRange(CameraControl_Exposure, &min, &max, &step, &def, &caps);
                    if (hr == DEVICE_NOT_READY_ERROR)
                        return false;

                    CHECK_HR(hr);

                    hr = _camera_control->Set(CameraControl_Exposure, def, CameraControl_Flags_Manual);
                    if (hr == DEVICE_NOT_READY_ERROR)
                        return false;

                    CHECK_HR(hr);
                }
                return true;
            }
            for (auto & pu : pu_controls)
            {
                if (opt == pu.option)
                {
                    if (pu.enable_auto)
                    {
                        if (value)
                        {
                            auto hr = _video_proc->Set(pu.property, 0, VideoProcAmp_Flags_Auto);
                            if (hr == DEVICE_NOT_READY_ERROR)
                                return false;

                            CHECK_HR(hr);
                        }
                        else
                        {
                            long min, max, step, def, caps;
                            auto hr = _video_proc->GetRange(pu.property, &min, &max, &step, &def, &caps);
                            if (hr == DEVICE_NOT_READY_ERROR)
                                return false;

                            CHECK_HR(hr);

                            hr = _video_proc->Set(pu.property, def, VideoProcAmp_Flags_Manual);
                            if (hr == DEVICE_NOT_READY_ERROR)
                                return false;

                            CHECK_HR(hr);
                        }
                    }
                    else
                    {
                        auto hr = _video_proc->Set(pu.property, value, VideoProcAmp_Flags_Manual);
                        if (hr == DEVICE_NOT_READY_ERROR)
                            return false;

                        CHECK_HR(hr);
                    }
                    return true;
                }
            }
            throw std::runtime_error("Unsupported control!");
        }

        control_range wmf_uvc_device::get_pu_range(rs2_option opt) const
        {
            if (opt == RS2_OPTION_ENABLE_AUTO_EXPOSURE ||
                opt == RS2_OPTION_ENABLE_AUTO_WHITE_BALANCE)
            {
                static const int32_t min = 0, max = 1, step = 1, def = 1;
                control_range result(min, max, step, def);
                return result;
            }

            long minVal = 0, maxVal = 0, steppingDelta = 0, defVal = 0, capsFlag = 0;
            if (opt == RS2_OPTION_EXPOSURE)
            {
                CHECK_HR(_camera_control->GetRange(CameraControl_Exposure, &minVal, &maxVal, &steppingDelta, &defVal, &capsFlag));
                control_range result(minVal, maxVal, steppingDelta, defVal);
                return result;
            }
            for (auto & pu : pu_controls)
            {
                if (opt == pu.option)
                {
                    if (!_video_proc.p) throw std::runtime_error("No video proc!");
                    CHECK_HR(_video_proc->GetRange(pu.property, &minVal, &maxVal, &steppingDelta, &defVal, &capsFlag));
                    control_range result(minVal, maxVal, steppingDelta, defVal);
                    return result;
                }
            }
            throw std::runtime_error("unsupported control");
        }

        void wmf_uvc_device::foreach_uvc_device(enumeration_callback action)
        {
            CComPtr<IMFAttributes> pAttributes = nullptr;
            CHECK_HR(MFCreateAttributes(&pAttributes, 1));
            CHECK_HR(pAttributes->SetGUID(
                MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
                MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID));

            IMFActivate ** ppDevices;
            UINT32 numDevices;
            CHECK_HR(MFEnumDeviceSources(pAttributes, &ppDevices, &numDevices));

            for (UINT32 i = 0; i < numDevices; ++i)
            {
                CComPtr<IMFActivate> pDevice;
                *&pDevice = ppDevices[i];

                WCHAR * wchar_name = nullptr; UINT32 length;
                CHECK_HR(pDevice->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK, &wchar_name, &length));
                auto name = win_to_utf(wchar_name);
                CoTaskMemFree(wchar_name);

                uint16_t vid, pid, mi; std::string unique_id;
                if (!parse_usb_path(vid, pid, mi, unique_id, name)) continue;

                uvc_device_info info;
                info.vid = vid;
                info.pid = pid;
                info.unique_id = unique_id;
                info.mi = mi;
                info.device_path = name;
                try
                {
                    action(info, ppDevices[i]);
                }
                catch (...)
                {
                    // TODO
                }
            }

            CoTaskMemFree(ppDevices);
        }

        void wmf_uvc_device::set_power_state(power_state state)
        {
            auto rs2 = is_win10_redstone2();

            // This is temporary work-around for Windows 10 Red-Stone2 build
            // There seem to be issues re-creating Media Foundation objects frequently
            // That's why until this is properly investigated on RS2 machines librealsense will keep MF objects alive
            // As a negative side-effect the camera will remain in D0 power state longer
            if (rs2)
            {
                if (_power_state != D0 && state == D0)
                {
                    foreach_uvc_device([this](const uvc_device_info& i,
                        IMFActivate* device)
                    {
                        if (i == _info && device)
                        {
                            wchar_t did[256];
                            HRESULT hr;
                            int count = 0;
                            CHECK_HR(device->GetString(did_guid, did, sizeof(did) / sizeof(wchar_t), nullptr));

                            if (_reader == nullptr)
                            {
                                CHECK_HR(MFCreateAttributes(&_device_attrs, 2));
                                CHECK_HR(_device_attrs->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, type_guid));
                                CHECK_HR(_device_attrs->SetString(did_guid, did));
                                hr = device->ActivateObject(IID_PPV_ARGS(&_source));
                                if (_source == nullptr)
                                {
                                    do
                                    {
                                        Sleep(10);
                                        count++;
                                        hr = device->ActivateObject(IID_PPV_ARGS(&_source));
                                    } while (_source == nullptr && count < 30);
                                }

                                CHECK_HR(hr);

                                _callback = new source_reader_callback(shared_from_this()); /// async I/O

                                CHECK_HR(MFCreateAttributes(&_reader_attrs, 3));
                                CHECK_HR(_reader_attrs->SetUINT32(MF_SOURCE_READER_DISCONNECT_MEDIASOURCE_ON_SHUTDOWN, FALSE));
                                CHECK_HR(_reader_attrs->SetUnknown(MF_SOURCE_READER_ASYNC_CALLBACK, static_cast<IUnknown*>(_callback)));
                                CHECK_HR(_reader_attrs->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE));
                                CHECK_HR(MFCreateSourceReaderFromMediaSource(_source, _reader_attrs, &_reader));
                            }
                            _source->QueryInterface(__uuidof(IAMCameraControl),
                                reinterpret_cast<void **>(&_camera_control.p));
                            _source->QueryInterface(__uuidof(IAMVideoProcAmp),
                                reinterpret_cast<void **>(&_video_proc.p));

                            UINT32 streamIndex;
                            CHECK_HR(_device_attrs->GetCount(&streamIndex));
                            _streams.resize(streamIndex);
                            for (auto& elem : _streams)
                            {
                                elem.callback = nullptr;
                            }

                            CHECK_HR(_reader->SetStreamSelection(static_cast<DWORD>(MF_SOURCE_READER_ALL_STREAMS), TRUE));
                        }
                    });

                    _power_state = D0;
                }

                if (_power_state != D3 && state == D3)
                {
                    _power_state = D3;
                }
            }
            else
            {
                if (_power_state != D0 && state == D0)
                {
                    foreach_uvc_device([this](const uvc_device_info& i,
                        IMFActivate* device)
                    {
                        if (i == _info && device)
                        {
                            wchar_t did[256];
                            CHECK_HR(device->GetString(did_guid, did, sizeof(did) / sizeof(wchar_t), nullptr));

                            CHECK_HR(MFCreateAttributes(&_device_attrs, 2));
                            CHECK_HR(_device_attrs->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, type_guid));
                            CHECK_HR(_device_attrs->SetString(did_guid, did));
                            CHECK_HR(MFCreateDeviceSourceActivate(_device_attrs, &_activate));

                            _callback = new source_reader_callback(shared_from_this()); /// async I/O

                            CHECK_HR(MFCreateAttributes(&_reader_attrs, 2));
                            CHECK_HR(_reader_attrs->SetUINT32(MF_SOURCE_READER_DISCONNECT_MEDIASOURCE_ON_SHUTDOWN, FALSE));
                            CHECK_HR(_reader_attrs->SetUnknown(MF_SOURCE_READER_ASYNC_CALLBACK,
                                static_cast<IUnknown*>(_callback)));

                            CHECK_HR(_reader_attrs->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE));
                            CHECK_HR(_activate->ActivateObject(IID_IMFMediaSource, reinterpret_cast<void **>(&_source)));
                            CHECK_HR(MFCreateSourceReaderFromMediaSource(_source, _reader_attrs, &_reader));

                            _source->QueryInterface(__uuidof(IAMCameraControl),
                                reinterpret_cast<void **>(&_camera_control.p));
                            _source->QueryInterface(__uuidof(IAMVideoProcAmp),
                                reinterpret_cast<void **>(&_video_proc.p));

                            UINT32 streamIndex;
                            CHECK_HR(_device_attrs->GetCount(&streamIndex));
                            _streams.resize(streamIndex);
                            for (auto& elem : _streams)
                            {
                                elem.callback = nullptr;
                            }

                            CHECK_HR(_reader->SetStreamSelection(static_cast<DWORD>(MF_SOURCE_READER_ALL_STREAMS), TRUE));
                        }
                    });

                    _power_state = D0;
                }

                if (_power_state != D3 && state == D3)
                {
                    _reader = nullptr;
                    _source = nullptr;
                    _reader_attrs = nullptr;
                    _activate = nullptr;
                    _device_attrs = nullptr;

                    _power_state = D3;
                }
            }
        }

        std::vector<stream_profile> wmf_uvc_device::get_profiles() const
        {
            check_connection();

            if (get_power_state() != D0)
                throw std::runtime_error("Device must be powered to query supported profiles!");

            CComPtr<IMFMediaType> pMediaType = nullptr;
            std::vector<stream_profile> results;
            for (unsigned int sIndex = 0; sIndex < _streams.size(); ++sIndex)
            {
                for (auto k = 0;; k++)
                {
                    auto hr = _reader->GetNativeMediaType(sIndex, k, &pMediaType.p);
                    if (FAILED(hr) || pMediaType == nullptr)
                    {
                        if (hr != MF_E_NO_MORE_TYPES) // An object ran out of media types to suggest therefore the requested chain of streaming objects cannot be completed
                            check("_reader->GetNativeMediaType(sIndex, k, &pMediaType.p)", hr, false);

                        break;
                    }

                    GUID subtype;
                    CHECK_HR(pMediaType->GetGUID(MF_MT_SUBTYPE, &subtype));

                    unsigned width = 0;
                    unsigned height = 0;

                    CHECK_HR(MFGetAttributeSize(pMediaType, MF_MT_FRAME_SIZE, &width, &height));

                    typedef struct frameRate {
                        unsigned int denominator;
                        unsigned int numerator;
                    } frameRate;

                    frameRate frameRateMin;
                    frameRate frameRateMax;

                    CHECK_HR(MFGetAttributeRatio(pMediaType, MF_MT_FRAME_RATE_RANGE_MIN, &frameRateMin.numerator, &frameRateMin.denominator));
                    CHECK_HR(MFGetAttributeRatio(pMediaType, MF_MT_FRAME_RATE_RANGE_MAX, &frameRateMax.numerator, &frameRateMax.denominator));

                    if (static_cast<float>(frameRateMax.numerator) / frameRateMax.denominator <
                        static_cast<float>(frameRateMin.numerator) / frameRateMin.denominator)
                    {
                        std::swap(frameRateMax, frameRateMin);
                    }
                    int currFps = frameRateMax.numerator / frameRateMax.denominator;

                    uint32_t device_fourcc = reinterpret_cast<const big_endian<uint32_t> &>(subtype.Data1);
                    if (fourcc_map.count(device_fourcc))
                        device_fourcc = fourcc_map.at(device_fourcc);

                    stream_profile sp;
                    sp.width = width;
                    sp.height = height;
                    sp.fps = currFps;
                    sp.format = device_fourcc;
                    results.push_back(sp);
                }
            }
            return results;
        }

        wmf_uvc_device::wmf_uvc_device(const uvc_device_info& info,
            std::shared_ptr<const wmf_backend> backend)
            : _info(info), _is_flushed(), _has_started(), _backend(std::move(backend)),
            _systemwide_lock(info.unique_id.c_str(), WAIT_FOR_MUTEX_TIME_OUT),
            _location("")
        {
            if (!is_connected(info))
            {
                throw std::runtime_error("Camera not connected!");
            }
            try
            {
                //_location = get_usb_port_id(info.vid, info.pid, info.unique_id);
            }
            catch (...) {}
        }


        wmf_uvc_device::~wmf_uvc_device()
        {
            try {
                if (_streaming)
                {
                    flush(MF_SOURCE_READER_ALL_STREAMS);
                }
                wmf_uvc_device::set_power_state(D3);

                if (_source)
                {
                    _source.Release();
                    _source = nullptr;
                    _reader.Release();
                    _reader = nullptr;
                    _reader_attrs.Release();
                    _reader_attrs = nullptr;
                    _device_attrs.Release();
                    _device_attrs = nullptr;
                }
            }
            catch (...)
            {
                // TODO: Log
            }
        }

        void wmf_uvc_device::probe_and_commit(stream_profile profile, frame_callback callback, int /*buffers*/)
        {
            if (_streaming)
                throw std::runtime_error("Device is already streaming!");

            _profiles.push_back(profile);
            _frame_callbacks.push_back(callback);
        }

        void wmf_uvc_device::play_profile(stream_profile profile, frame_callback callback)
        {
            CComPtr<IMFMediaType> pMediaType = nullptr;
            for (unsigned int sIndex = 0; sIndex < _streams.size(); ++sIndex)
            {
                for (auto k = 0;; k++)
                {
                    auto hr = _reader->GetNativeMediaType(sIndex, k, &pMediaType.p);
                    if (FAILED(hr) || pMediaType == nullptr)
                    {
                        if (hr != MF_E_NO_MORE_TYPES) // An object ran out of media types to suggest therefore the requested chain of streaming objects cannot be completed
                            check("_reader->GetNativeMediaType(sIndex, k, &pMediaType.p)", hr, false);

                        break;
                    }

                    GUID subtype;
                    CHECK_HR(pMediaType->GetGUID(MF_MT_SUBTYPE, &subtype));

                    uint32_t device_fourcc = reinterpret_cast<const big_endian<uint32_t> &>(subtype.Data1);

                    if (device_fourcc != profile.format &&
                        (fourcc_map.count(device_fourcc) == 0 ||
                            profile.format != fourcc_map.at(device_fourcc)))
                        continue;

                    unsigned int width;
                    unsigned int height;

                    CHECK_HR(MFGetAttributeSize(pMediaType, MF_MT_FRAME_SIZE, &width, &height));

                    typedef struct frameRate {
                        unsigned int denominator;
                        unsigned int numerator;
                    } frameRate;

                    frameRate frameRateMin;
                    frameRate frameRateMax;

                    CHECK_HR(MFGetAttributeRatio(pMediaType, MF_MT_FRAME_RATE_RANGE_MIN, &frameRateMin.numerator, &frameRateMin.denominator));
                    CHECK_HR(MFGetAttributeRatio(pMediaType, MF_MT_FRAME_RATE_RANGE_MAX, &frameRateMax.numerator, &frameRateMax.denominator));

                    if ((width == profile.width) && (height == profile.height))
                    {
                        if (frameRateMax.denominator && frameRateMin.denominator)
                        {
                            if (static_cast<float>(frameRateMax.numerator) / frameRateMax.denominator <
                                static_cast<float>(frameRateMin.numerator) / frameRateMin.denominator)
                            {
                                std::swap(frameRateMax, frameRateMin);
                            }
                            int currFps = frameRateMax.numerator / frameRateMax.denominator;
                            if (currFps == int(profile.fps))
                            {
                                hr = _reader->SetCurrentMediaType(sIndex, nullptr, pMediaType);

                                if (SUCCEEDED(hr) && pMediaType)
                                {
                                    for (unsigned int i = 0; i < _streams.size(); ++i)
                                    {
                                        if (sIndex == i || (_streams[i].callback))
                                            continue;

                                        _reader->SetStreamSelection(i, FALSE);
                                    }

                                    CHECK_HR(_reader->SetStreamSelection(sIndex, TRUE));

                                    {
                                        std::lock_guard<std::mutex> lock(_streams_mutex);
                                        if (_streams[sIndex].callback)
                                            throw std::runtime_error("Camera already streaming via this stream index!");

                                        _streams[sIndex].profile = profile;
                                        _streams[sIndex].callback = callback;
                                    }

                                    _readsample_result = S_OK;
                                    CHECK_HR(_reader->ReadSample(sIndex, 0, nullptr, nullptr, nullptr, nullptr));

                                    const auto timeout_ms = 5000;
                                    if (_has_started.wait(timeout_ms))
                                    {
                                        check("_reader->ReadSample(...)", _readsample_result);
                                    }
                                    else
                                    {
                                        LOG_WARNING("First frame took more then " << timeout_ms << "ms to arrive!");
                                    }

                                    return;
                                }
                                else
                                {
                                    throw std::runtime_error("Could not set Media Type. Device may be locked");
                                }
                            }
                        }
                    }
                }
            }
            throw std::runtime_error("Stream profile not found!");

        }

        void wmf_uvc_device::stream_on(std::function<void(const notification& n)> error_handler)
        {
            if (_profiles.empty())
                throw std::runtime_error("Stream not configured");

            if (_streaming)
                throw std::runtime_error("Device is already streaming!");

            check_connection();

            try
            {
                for (uint32_t i = 0; i < _profiles.size(); ++i)
                {
                    play_profile(_profiles[i], _frame_callbacks[i]);
                }

                _streaming = true;
            }
            catch (...)
            {
                for (auto& elem : _streams)
                    if (elem.callback)
                        close(elem.profile);

                _profiles.clear();
                _frame_callbacks.clear();

                throw;
            }
        }

        void wmf_uvc_device::start_callbacks()
        {
            _is_started = true;
        }

        void wmf_uvc_device::stop_callbacks()
        {
            _is_started = false;
        }

        void wmf_uvc_device::stop_stream_cleanup(const stream_profile& profile, std::vector<profile_and_callback>::iterator& elem)
        {
            if (elem != _streams.end())
            {
                elem->callback = nullptr;
                elem->profile.format = 0;
                elem->profile.fps = 0;
                elem->profile.width = 0;
                elem->profile.height = 0;
            }

            auto pos = std::find(_profiles.begin(), _profiles.end(), profile) - _profiles.begin();
            if (pos != _profiles.size())
            {
                _profiles.erase(_profiles.begin() + pos);
                _frame_callbacks.erase(_frame_callbacks.begin() + pos);
            }

            if (_profiles.empty())
                _streaming = false;

            _has_started.reset();
        }

        void wmf_uvc_device::close(stream_profile profile)
        {
            _is_started = false;

            check_connection();

            auto& elem = std::find_if(_streams.begin(), _streams.end(),
                [&](const profile_and_callback& pac) {
                return (pac.profile == profile && (pac.callback));
            });

            if (elem == _streams.end() && _frame_callbacks.empty())
                throw std::runtime_error("Camera is not streaming!");

            if (elem != _streams.end())
            {
                try {
                    flush(int(elem - _streams.begin()));
                }
                catch (...)
                {
                    stop_stream_cleanup(profile, elem); // TODO: move to RAII
                    throw;
                }
            }
            stop_stream_cleanup(profile, elem);
        }

        // ReSharper disable once CppMemberFunctionMayBeConst
        void wmf_uvc_device::flush(int sIndex)
        {
            if (is_connected(_info))
            {
                if (_reader != nullptr)
                {
                    auto sts = _reader->Flush(sIndex);
                    if (sts != S_OK)
                    {
                        if (sts == MF_E_HW_MFT_FAILED_START_STREAMING)
                            throw std::runtime_error("Camera already streaming");

                        throw;
                    }

                    _is_flushed.wait(INFINITE);
                }
            }
        }

        void wmf_uvc_device::check_connection() const
        {
            if (!is_connected(_info))
                throw std::runtime_error("Camera is no longer connected!");
        }
    }
}

#endif
