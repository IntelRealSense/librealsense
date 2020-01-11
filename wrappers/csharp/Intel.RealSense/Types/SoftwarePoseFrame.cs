// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    using System;
    using System.Runtime.InteropServices;

    [StructLayout(LayoutKind.Sequential)]
    public class SoftwarePoseFrame
    {
        [System.Serializable]
        // [StructLayout(LayoutKind.Sequential)]
        [StructLayout(LayoutKind.Sequential, Pack = 1, Size = 84)]
        // [StructLayout(LayoutKind.Explicit, Pack = 1, Size = 84)]
        public class PoseFrameInfo
        {
            // [FieldOffset(0)]
            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 3)]
            public float[] translation;

            // [FieldOffset(12)]
            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 3)]
            public float[] velocity;

            // [FieldOffset(24)]
            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 3)]
            public float[] acceleration;

            // [FieldOffset(36)]
            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 4)]
            public float[] rotation;

            // [FieldOffset(52)]
            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 3)]
            public float[] angular_velocity;

            // [FieldOffset(64)]
            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 3)]
            public float[] angular_acceleration;

            // [FieldOffset(76)]
            public int tracker_confidence;

            // [FieldOffset(80)]
            public int mapper_confidence;
        }

        public IntPtr data;
        public frame_deleter deleter = delegate { };
        public int stride;
        public int bpp;
        public double timestamp;
        public TimestampDomain domain;
        public int frame_number;
    }
}
