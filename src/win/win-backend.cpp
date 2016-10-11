// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.
#ifdef RS_USE_WMF_BACKEND

#if (_MSC_FULL_VER < 180031101)
    #error At least Visual Studio 2013 Update 4 is required to compile this backend
#endif

#include "win-backend.h"
#include "win-uvc.h"

#include <mfapi.h>

namespace rsimpl
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
            MFShutdown();
            CoUninitialize();
        }

        std::shared_ptr<uvc_device> wmf_backend::create_uvc_device(uvc_device_info info) const
        {
            return std::shared_ptr<uvc_device>(new wmf_uvc_device(info, shared_from_this()));
        }

        std::shared_ptr<backend> create_backend()
        {
            return std::shared_ptr<backend>(new wmf_backend());
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
    }
}

#endif
