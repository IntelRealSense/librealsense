// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    using System;
    using System.Collections.Generic;
    using System.Runtime.InteropServices;

    public class DepthFrame : VideoFrame
    {
        public DepthFrame(IntPtr ptr)
            : base(ptr)
        {
        }

        /// <summary>Given the 2D depth coordinate (x,y) provide the corresponding depth in metric units</summary>
        /// <returns>depth in metric units</returns>
        public float GetDistance(int x, int y)
        {
            object error;
            return NativeMethods.rs2_depth_frame_get_distance(Handle, x, y, out error);
        }
    }
}
