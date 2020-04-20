// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    using System;
    using System.Runtime.InteropServices;
    using Intel.RealSense;

    public class MotionFrame : Frame
    {
        public MotionFrame(IntPtr ptr)
            : base(ptr)
        {
        }

        public Math.Vector MotionData
        {
            get
            {
                Math.Vector xyz;
                CopyTo(out xyz);
                return xyz;
            }
        }

        public void CopyTo(float[] data)
        {
            Marshal.Copy(Data, data, 0, 3);
        }

        public void CopyTo<T>(out T xyz)
            where T : struct
        {
            xyz = (T)Marshal.PtrToStructure(Data, typeof(T));
        }

        public void CopyTo<T>(T xyz)
            where T : class
        {
            Marshal.PtrToStructure(Data, xyz);
        }
    }
}
