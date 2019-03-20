// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    using System;
    using System.Collections.Generic;
    using System.Runtime.InteropServices;

    public class DisparityFrame : DepthFrame
    {
        public DisparityFrame(IntPtr ptr)
            : base(ptr)
        {
        }

        public float Baseline
        {
            get
            {
                object error;
                return NativeMethods.rs2_depth_stereo_frame_get_baseline(Handle, out error);
            }
        }
    }
}
