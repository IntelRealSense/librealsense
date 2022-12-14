// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    using System;
    using System.Runtime.InteropServices;

    public class MaxUsableRangeSensor : Sensor
    {
        internal MaxUsableRangeSensor(IntPtr ptr)
            : base(ptr)
        {
        }

    /// <summary>
    /// Returns the maximum range of the camera given the amount of ambient light in the scene [m]
    /// </summary>
    /// <returns>maximum usable range</returns>
    public float GetMaxUsableRange( )
        {
            object error;
            return NativeMethods.rs2_get_max_usable_depth_range(Handle, out error);
        }
    }
}
