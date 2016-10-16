// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#ifdef RS_USE_WMF_BACKEND

#if (_MSC_FULL_VER < 180031101)
#error At least Visual Studio 2013 Update 4 is required to compile this backend
#endif

#include "win-uvc.h"

#include "Shlwapi.h"
#include <Windows.h>
#include "mfapi.h"
#include <vidcap.h>

#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")


#define type_guid  MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID 
#define did_guid  MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK


namespace rsimpl
{
    namespace uvc
    {
        class source_reader_callback : public IMFSourceReaderCallback
        {
        public:
            explicit source_reader_callback(std::weak_ptr<wmf_uvc_device> owner) 
                : _owner(owner)
            {
            }

            virtual ~source_reader_callback(void) {};

            STDMETHODIMP QueryInterface(REFIID iid, void** ppv) override
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

            STDMETHODIMP_(ULONG) AddRef() override { return InterlockedIncrement(&_refCount); }

            STDMETHODIMP_(ULONG) Release() override {
                ULONG count = InterlockedDecrement(&_refCount);
                if (count <= 0)
                {
                    delete this;
                }
                return count;
            }

            STDMETHODIMP OnReadSample(HRESULT /*hrStatus*/, 
                                      DWORD dwStreamIndex, 
                                      DWORD /*dwStreamFlags*/, 
                                      LONGLONG /*llTimestamp*/, 
                                      IMFSample *sample) override
            {
                auto owner = _owner.lock();
                if (owner)
                {
                    LOG_HR(owner->_reader->ReadSample(0xFFFFFFFC, 0, nullptr, nullptr, nullptr, nullptr));

                    if (sample)
                    {
                        CComPtr<IMFMediaBuffer> buffer = nullptr;
                        if (SUCCEEDED(sample->GetBufferByIndex(0, &buffer)))
                        {
                            byte* byte_buffer;
                            DWORD max_length, current_length;
                            if (SUCCEEDED(buffer->Lock(&byte_buffer, &max_length, &current_length)))
                            {
                                try
                                {
                                    auto& stream = owner->_streams[dwStreamIndex];
                                    std::lock_guard<std::mutex> lock(owner->_streams_mutex);
                                    auto profile = stream.profile;
                                    frame_object f{ byte_buffer };
                                    stream.callback(profile, f);
                                }
                                catch (...)
                                {
                                    // TODO: log
                                }

                                buffer->Unlock();
                            }
                        }
                    }
                }
                
                return S_OK;
            };
            STDMETHODIMP OnEvent(DWORD /*sidx*/, IMFMediaEvent* /*event*/) override { return S_OK; }
            STDMETHODIMP OnFlush(DWORD) override
            {
                auto owner = _owner.lock();
                if (owner)
                {
                    owner->_is_flushed.set();
                }
                return S_OK;
            }

        private:
            std::weak_ptr<wmf_uvc_device> _owner;
            long _refCount = 0;
        };

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
            // Attempt to retrieve IKsControl
            CComPtr<IKsTopologyInfo> ks_topology_info = nullptr;
            CHECK_HR(_source->QueryInterface(__uuidof(IKsTopologyInfo), 
                     reinterpret_cast<void **>(&ks_topology_info)));

            GUID node_type;
            CHECK_HR(ks_topology_info->get_NodeType(xu.node, &node_type));
            const GUID KSNODETYPE_DEV_SPECIFIC_LOCAL{ 
                0x941C7AC0L, 0xC559, 0x11D0,
                { 0x8A, 0x2B, 0x00, 0xA0, 0xC9, 0x25, 0x5A, 0xC1 } 
            };
            if (node_type != KSNODETYPE_DEV_SPECIFIC_LOCAL) 
                throw std::runtime_error(to_string() << 
                    "Invalid extension unit node ID: " << xu.node);

            CComPtr<IUnknown> unknown;
            CHECK_HR(ks_topology_info->CreateNodeInstance(xu.node, IID_IUnknown, 
                reinterpret_cast<LPVOID *>(&unknown)));

            CComPtr<IKsControl> ks_control;
            CHECK_HR(unknown->QueryInterface(__uuidof(IKsControl), 
                reinterpret_cast<void **>(&ks_control)));
            LOG_INFO("Obtained KS control node " << xu.node);
            _ks_controls[xu.node] = ks_control;
        }

        void wmf_uvc_device::set_xu(const extension_unit& xu, uint8_t ctrl, void* data, int len)
        {
            auto ks_control = get_ks_control(xu);

            KSP_NODE node;
            memset(&node, 0, sizeof(KSP_NODE));
            node.Property.Set = reinterpret_cast<const GUID &>(xu.id);
            node.Property.Id = ctrl;
            node.Property.Flags = KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_TOPOLOGY;
            node.NodeId = xu.node;

            ULONG bytes_received = 0;
            CHECK_HR(ks_control->KsProperty(reinterpret_cast<PKSPROPERTY>(&node), sizeof(KSP_NODE), data, len, &bytes_received));
        }

        void wmf_uvc_device::get_xu(const extension_unit& xu, uint8_t ctrl, void* data, int len) const
        {
            auto ks_control = get_ks_control(xu);

            KSP_NODE node;
            memset(&node, 0, sizeof(KSP_NODE));
            node.Property.Set = reinterpret_cast<const GUID &>(xu.id);
            node.Property.Id = ctrl;
            node.Property.Flags = KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_TOPOLOGY;
            node.NodeId = xu.node;

            ULONG bytes_received = 0;
            CHECK_HR(ks_control->KsProperty(reinterpret_cast<PKSPROPERTY>(&node), sizeof(node), data, len, &bytes_received));
            if (bytes_received != len) 
                throw std::runtime_error("XU read did not return enough data");
        }

        control_range wmf_uvc_device::get_xu_range(const extension_unit& xu, uint8_t ctrl) const
        {
            control_range result;
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

            auto pRangeValues = buffer.data() + sizeof(KSPROPERTY_MEMBERSHEADER) + sizeof(KSPROPERTY_DESCRIPTION);

            result.step = static_cast<int>(*pRangeValues);
            pRangeValues++;
            result.min = static_cast<int>(*pRangeValues);
            pRangeValues++;
            result.max = static_cast<int>(*pRangeValues);


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

            pRangeValues = buffer.data() + sizeof(KSPROPERTY_MEMBERSHEADER) + sizeof(KSPROPERTY_DESCRIPTION);

            result.def = static_cast<int>(*pRangeValues);
            return result;
        }

        struct pu_control { rs_option option; long property; bool enable_auto; };
        static const pu_control pu_controls[] = {
            { RS_OPTION_COLOR_BACKLIGHT_COMPENSATION, VideoProcAmp_BacklightCompensation },
            { RS_OPTION_COLOR_BRIGHTNESS, VideoProcAmp_Brightness },
            { RS_OPTION_COLOR_CONTRAST, VideoProcAmp_Contrast },
            { RS_OPTION_COLOR_GAIN, VideoProcAmp_Gain },
            { RS_OPTION_COLOR_GAMMA, VideoProcAmp_Gamma },
            { RS_OPTION_COLOR_HUE, VideoProcAmp_Hue },
            { RS_OPTION_COLOR_SATURATION, VideoProcAmp_Saturation },
            { RS_OPTION_COLOR_SHARPNESS, VideoProcAmp_Sharpness },
            { RS_OPTION_COLOR_WHITE_BALANCE, VideoProcAmp_WhiteBalance },
            { RS_OPTION_COLOR_ENABLE_AUTO_WHITE_BALANCE, VideoProcAmp_WhiteBalance, true },
            { RS_OPTION_FISHEYE_GAIN, VideoProcAmp_Gain }
        };

        int wmf_uvc_device::get_pu(rs_option opt) const
        {
            long value = 0, flags = 0;
            if (opt == RS_OPTION_COLOR_EXPOSURE)
            {
                CHECK_HR(_camera_control->Get(CameraControl_Exposure, &value, &flags));
                return value;
            }
            if (opt == RS_OPTION_COLOR_ENABLE_AUTO_EXPOSURE)
            {
                CHECK_HR(_camera_control->Get(CameraControl_Exposure, &value, &flags));
                return flags == CameraControl_Flags_Auto;
            }
            for (auto & pu : pu_controls)
            {
                if (opt == pu.option)
                {
                    CHECK_HR(_video_proc->Get(pu.property, &value, &flags));
                    if (pu.enable_auto) return flags == VideoProcAmp_Flags_Auto;
                    else return value;
                }
            }
            throw std::runtime_error("Unsupported control!");
        }

        void wmf_uvc_device::set_pu(rs_option opt, int value)
        {
            if (opt == RS_OPTION_COLOR_EXPOSURE)
            {
                CHECK_HR(_camera_control->Set(CameraControl_Exposure, static_cast<int>(value), CameraControl_Flags_Manual));
                return;
            }
            if (opt == RS_OPTION_COLOR_ENABLE_AUTO_EXPOSURE)
            {
                if (value)
                {
                    CHECK_HR(_camera_control->Set(CameraControl_Exposure, 0, CameraControl_Flags_Auto));
                }
                else
                {
                    long min, max, step, def, caps;
                    CHECK_HR(_camera_control->GetRange(CameraControl_Exposure, &min, &max, &step, &def, &caps));
                    CHECK_HR(_camera_control->Set(CameraControl_Exposure, def, CameraControl_Flags_Manual));
                }
                return;
            }
            for (auto & pu : pu_controls)
            {
                if (opt == pu.option)
                {
                    if (pu.enable_auto)
                    {
                        if (value)
                        {
                            CHECK_HR(_video_proc->Set(pu.property, 0, VideoProcAmp_Flags_Auto));
                        }
                        else
                        {
                            long min, max, step, def, caps;
                            CHECK_HR(_video_proc->GetRange(pu.property, &min, &max, &step, &def, &caps));
                            CHECK_HR(_video_proc->Set(pu.property, def, VideoProcAmp_Flags_Manual));
                        }
                    }
                    else
                    {
                        CHECK_HR(_video_proc->Set(pu.property, value, VideoProcAmp_Flags_Manual));
                    }
                    return;
                }
            }
            throw std::runtime_error("Unsupported control!");
        }

        control_range wmf_uvc_device::get_pu_range(rs_option opt) const
        {
            control_range result;
            if (opt == RS_OPTION_COLOR_ENABLE_AUTO_EXPOSURE || 
                opt == RS_OPTION_COLOR_ENABLE_AUTO_WHITE_BALANCE)
            {
                result.min = 0;
                result.max = 1;
                result.step = 1;
                result.def = 1;
                return result;
            }

            long minVal = 0, maxVal = 0, steppingDelta = 0, defVal = 0, capsFlag = 0;
            if (opt == RS_OPTION_COLOR_EXPOSURE)
            {
                CHECK_HR(_camera_control->GetRange(CameraControl_Exposure, &minVal, &maxVal, &steppingDelta, &defVal, &capsFlag));
                result.min = minVal;
                result.max = maxVal;
                result.step = steppingDelta;
                result.def = defVal;
                return result;
            }
            for (auto & pu : pu_controls)
            {
                if (opt == pu.option)
                {
                    CHECK_HR(_video_proc->GetRange(pu.property, &minVal, &maxVal, &steppingDelta, &defVal, &capsFlag));
                    result.min = static_cast<int>(minVal);
                    result.max = static_cast<int>(maxVal);
                    result.step = static_cast<int>(steppingDelta);
                    result.def = static_cast<int>(defVal);
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

            for (UINT32 i = 0; i<numDevices; ++i)
            {
                CComPtr<IMFActivate> pDevice;
                *&pDevice = ppDevices[i];

                WCHAR * wchar_name = nullptr; UINT32 length;
                CHECK_HR(pDevice->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK, &wchar_name, &length));
                auto name = win_to_utf(wchar_name);
                CoTaskMemFree(wchar_name);

                int vid, pid, mi; std::string unique_id;
                if (!parse_usb_path(vid, pid, mi, unique_id, name)) continue;

                uvc_device_info info;
                info.vid = vid;
                info.pid = pid;
                info.unique_id = unique_id;
                info.mi = mi;
                try
                {
                    action(info, ppDevices[i]);
                }
                catch(...)
                {
                    // TODO
                }
            }

            CoTaskMemFree(ppDevices);
        }

        void wmf_uvc_device::set_power_state(power_state state)
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

                        CHECK_HR(_source->QueryInterface(__uuidof(IAMCameraControl), 
                                                         reinterpret_cast<void **>(&_camera_control.p)));
                        CHECK_HR(_source->QueryInterface(__uuidof(IAMVideoProcAmp), 
                                                         reinterpret_cast<void **>(&_video_proc.p)));

                        UINT32 streamIndex;
                        CHECK_HR(_device_attrs->GetCount(&streamIndex));
                        _streams.resize(streamIndex);
                        for (auto& elem : _streams)
                        {
                            elem.callback = nullptr;
                        }
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

        std::vector<stream_profile> wmf_uvc_device::get_profiles() const
        {
            std::vector<stream_profile> results;
            check_connection();

            CComPtr<IMFMediaType> pMediaType = nullptr;

            if (get_power_state() != D0)
                throw std::runtime_error("Device must be powered to query supported profiles!");

            CHECK_HR(_reader->SetStreamSelection(static_cast<DWORD>(MF_SOURCE_READER_ALL_STREAMS), TRUE));

            for (unsigned int sIndex = 0; sIndex < _streams.size(); ++sIndex)
            {
                for (auto k = 0;; k++)
                {
                    auto hr = _reader->GetNativeMediaType(sIndex, k, &pMediaType.p);
                    if (FAILED(hr) || pMediaType == nullptr)
                    {
                        LOG_HR(hr);
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

                    OLECHAR* bstrGuid;
                    CHECK_HR(StringFromCLSID(subtype, &bstrGuid));
                    char ch[260];
                    auto DefChar = ' ';
                    WideCharToMultiByte(CP_ACP, 0, bstrGuid, -1, ch, 260, &DefChar, nullptr);

                    std::string format(ch);

                    stream_profile sp;
                    sp.width = width;
                    sp.height = height;
                    sp.fps = currFps;
                    sp.format = format;
                    results.push_back(sp);
                }
            }
            return results;
        }

        wmf_uvc_device::wmf_uvc_device(const uvc_device_info& info,
                                       std::shared_ptr<const wmf_backend> backend)
            : _info(info), _is_flushed(), _backend(std::move(backend)),
              _systemwide_lock(info.unique_id.c_str(), WAIT_FOR_MUTEX_TIME_OUT)
        {
            if (!is_connected(info))
            {
                throw std::runtime_error("Camera not connected!");
            }
        }

        wmf_uvc_device::~wmf_uvc_device()
        {
            try {
                flush(MF_SOURCE_READER_ALL_STREAMS);
                wmf_uvc_device::set_power_state(D3);
            }
            catch (...)
            {
                // TODO: Log
            }
        }

        void wmf_uvc_device::play(stream_profile profile, frame_callback callback)
        {
            check_connection();

            GUID formatGuid;
            std::wstring formatWStr(profile.format.begin(), profile.format.end());
            CHECK_HR(CLSIDFromString(formatWStr.c_str(), static_cast<LPCLSID>(&formatGuid)));

            set_power_state(D0);

            CComPtr<IMFMediaType> pMediaType = nullptr;
            for (unsigned int sIndex = 0; sIndex < _streams.size(); ++sIndex)
            {
                for (auto k = 0;; k++)
                {
                    auto hr = _reader->GetNativeMediaType(sIndex, k, &pMediaType.p);
                    if (FAILED(hr) || pMediaType == nullptr)
                    {
                        LOG_HR(hr);
                        break;
                    }

                    GUID subtype;
                    CHECK_HR(pMediaType->GetGUID(MF_MT_SUBTYPE, &subtype));

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
                                if (subtype == formatGuid)
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

                                        CHECK_HR(_reader->ReadSample(sIndex, 0, nullptr, nullptr, nullptr, nullptr));
                                        return;
                                    }
                                }
                            }
                        }
                    }
                }
            }
            throw std::runtime_error("Stream profile not found!");
        }

        void wmf_uvc_device::stop(stream_profile profile)
        {
            check_connection();

            auto elem = std::find_if(_streams.begin(), _streams.end(), 
            [&](const profile_and_callback& pac) {
                return (pac.profile == profile && (pac.callback));
            });

            if (elem == _streams.end())
                throw std::runtime_error("Camera not streaming!");
            
            flush(int(elem - _streams.begin()));

            elem->callback = nullptr;
            elem->profile.format = "";
            elem->profile.fps = 0;
            elem->profile.width = 0;
            elem->profile.height = 0;
        }

        // ReSharper disable once CppMemberFunctionMayBeConst
        void wmf_uvc_device::flush(int sIndex)
        {
            if (is_connected(_info))
            {
                if (_reader != nullptr)
                {
                    CHECK_HR(_reader->Flush(sIndex));
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
