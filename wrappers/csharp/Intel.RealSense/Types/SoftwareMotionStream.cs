// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Runtime.InteropServices;
    using System.Text;

    [System.Serializable]
    [StructLayout(LayoutKind.Sequential)]
    public struct SoftwareMotionStream
    {
        public Stream type;
        public int index;
        public int uid;
        public int fps;
        public Format fmt;
        public MotionDeviceIntrinsics intrinsics;
    }
}
