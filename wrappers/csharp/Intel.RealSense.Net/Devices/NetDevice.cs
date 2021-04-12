// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense.Net
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Runtime.InteropServices;
    using Intel.RealSense;

    public class NetDevice : ProxyDevice
    {
        public NetDevice(string address) : base(init(address))
        {
        }

        public void AddTo(Context ctx)
        {
            object error;
            NativeMethods.rs2_context_add_software_device(ctx.Handle, Handle, out error);
        }

        private static IntPtr init(string address)
        {
            object error;
            var ApiVersion = NativeMethods.rs2_get_api_version(out error);
            var dev = NativeMethods.rs2_create_net_device(ApiVersion, address, out error);

            return dev;
        }
    }
}
