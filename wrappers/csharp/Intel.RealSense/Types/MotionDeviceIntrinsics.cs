// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    using System;
    using System.Runtime.InteropServices;

    /// <summary>
    /// Motion device intrinsics: scale, bias, and variances
    /// </summary>
    [System.Serializable]
    [StructLayout(LayoutKind.Sequential)]
    public struct MotionDeviceIntrinsics
    {
        /// <summary> Interpret data array values </summary>
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 3 * 4)]
        public float[] data;

        /// <summary> Variance of noise for X, Y, and Z axis </summary>
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 3)]
        public float[] noise_variances;

        /// <summary> Variance of bias for X, Y, and Z axis </summary>
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 3)]
        public float[] bias_variances;
    }
}
