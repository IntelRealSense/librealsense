// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    using System;
    using System.Runtime.InteropServices;

    [StructLayout(LayoutKind.Sequential)]
    public class SoftwareMotionFrame
    {
        public IntPtr data;
        public frame_deleter deleter = delegate { };
        public double timestamp;
        public TimestampDomain domain;
        public int frame_number;
        public IntPtr profile;
    }
}
