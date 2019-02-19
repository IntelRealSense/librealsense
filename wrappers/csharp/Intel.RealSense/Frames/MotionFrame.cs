using System;
using Intel.RealSense;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    public class MotionFrame : Frame
    {
        public MotionFrame(IntPtr ptr) : base(ptr)
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

        public void CopyTo<T>(out T xyz) where T : struct
        {
            xyz = (T)Marshal.PtrToStructure(Data, typeof(T));
        }

        public void CopyTo<T>(T xyz) where T : class
        {
            Marshal.PtrToStructure(Data, xyz);
        }
    }
}
