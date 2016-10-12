// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.
#pragma once

#include "../backend.h"
#include "win-helpers.h"

#include <mfidl.h>
#include <mfreadwrite.h>
#include <atlcomcli.h>

namespace rsimpl
{
    namespace uvc
    {
        class source_reader_callback;
        class wmf_backend;

        struct profile_and_callback
        {
            stream_profile profile;
            frame_callback callback = nullptr;
        };

        typedef std::function<void(const uvc_device_info&, IMFActivate*)> 
                enumeration_callback;

        class wmf_uvc_device : public std::enable_shared_from_this<wmf_uvc_device>, public uvc_device
        {
        public:
            wmf_uvc_device(const uvc_device_info& info, std::shared_ptr<const wmf_backend> backend);
            ~wmf_uvc_device();

            void play(stream_profile profile, frame_callback callback) override;
            void stop(stream_profile profile) override;
            void set_power_state(power_state state) override;
            power_state get_power_state() const override { return _powerState; }
            std::vector<stream_profile> get_profiles() const override;
            const uvc_device_info& get_info() const override { return _info; }

            static bool is_connected(const uvc_device_info& info);
            static void foreach_uvc_device(enumeration_callback action);

        private:
            friend class source_reader_callback;

            void flush(int sIndex);
            void check_connection() const;

            const uvc_device_info                   _info;
            power_state                             _powerState = D3;

            CComPtr<source_reader_callback>         _callback = nullptr;
            CComPtr<IMFSourceReader>                _reader = nullptr;
            CComPtr<IMFMediaSource>                 _pSource = nullptr;
            CComPtr<IMFActivate>                    _pActivate = nullptr;
            CComPtr<IMFAttributes>                  _pDeviceAttrs = nullptr;
            CComPtr<IMFAttributes>                  _pReaderAttrs = nullptr;
            HANDLE                                  _isFlushed = nullptr;

            std::vector<profile_and_callback>       _streams;
            std::mutex                              _streams_mutex;

            std::shared_ptr<const wmf_backend>      _backend;
        };
    }
}
