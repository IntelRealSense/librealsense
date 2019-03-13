using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    /// <summary>
    /// Cross-stream extrinsics: encode the topology describing how the different devices are connected.
    /// </summary>
    [System.Serializable]
    [StructLayout(LayoutKind.Sequential)]
    public struct Extrinsics
    {
        /// <summary>
        /// Column-major 3x3 rotation matrix
        /// </summary>
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 9)]
        public float[] rotation;

        /// <summary>
        /// Three-element translation vector, in meters
        /// </summary>
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 3)]
        public float[] translation;
    }
}
