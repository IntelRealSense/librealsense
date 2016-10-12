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
                    SetEvent(owner->_isFlushed);
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
            if (_powerState != D0 && state == D0)
            {
                foreach_uvc_device([this](const uvc_device_info& i, 
                                          IMFActivate* device)
                {
                    if (i == _info && device)
                    {
                        wchar_t did[256];
                        CHECK_HR(device->GetString(did_guid, did, sizeof(did) / sizeof(wchar_t), nullptr));

                        CHECK_HR(MFCreateAttributes(&_pDeviceAttrs, 2));
                        CHECK_HR(_pDeviceAttrs->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, type_guid));
                        CHECK_HR(_pDeviceAttrs->SetString(did_guid, did));
                        CHECK_HR(MFCreateDeviceSourceActivate(_pDeviceAttrs, &_pActivate));

                        _callback = new source_reader_callback(shared_from_this()); /// async I/O

                        CHECK_HR(MFCreateAttributes(&_pReaderAttrs, 2));
                        CHECK_HR(_pReaderAttrs->SetUINT32(MF_SOURCE_READER_DISCONNECT_MEDIASOURCE_ON_SHUTDOWN, FALSE));
                        CHECK_HR(_pReaderAttrs->SetUnknown(MF_SOURCE_READER_ASYNC_CALLBACK,
                                                           static_cast<IUnknown*>(_callback)));

                        CHECK_HR(_pReaderAttrs->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE));
                        CHECK_HR(_pActivate->ActivateObject(IID_IMFMediaSource, reinterpret_cast<void **>(&_pSource)));
                        CHECK_HR(MFCreateSourceReaderFromMediaSource(_pSource, _pReaderAttrs, &_reader));

                        UINT32 streamIndex;
                        CHECK_HR(_pDeviceAttrs->GetCount(&streamIndex));
                        _streams.resize(streamIndex);
                        for (auto& elem : _streams)
                        {
                            elem.callback = nullptr;
                        }
                    }
                });

                _powerState = D0;
            }

            if (_powerState != D3 && state == D3)
            {
                _reader = nullptr;
                _pSource = nullptr;
                _pReaderAttrs = nullptr;
                _pActivate = nullptr;
                _pDeviceAttrs = nullptr;

                _powerState = D3;
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
            : _info(info), _backend(backend)
        {
            _isFlushed = CreateEvent(nullptr, FALSE, FALSE, nullptr);

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

                CloseHandle(_isFlushed);
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
                    WaitForSingleObject(_isFlushed, INFINITE);
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
