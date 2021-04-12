// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.
//
// This proxy device is for accessing Device constructor that is internal, by successors in different namespaces and assemblies
// This way is more controllable for accessing base Device object instead of making it's constructors public
// Example of usage: NetDevice in Intel.Realsense.Net.dll that is a successor of ProxyDevice

namespace Intel.RealSense
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Runtime.InteropServices;


    public class ProxyDevice : Device
    {
        public ProxyDevice(IntPtr ptr) : base(ptr)
        {
        }
    }
}
