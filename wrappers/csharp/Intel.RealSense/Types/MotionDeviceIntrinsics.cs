using System;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    [System.Serializable]
    [StructLayout(LayoutKind.Sequential)]
    public struct MotionDeviceIntrinsics
    {
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 3*4)]
        ///<summary> Interpret data array values </summary>                        
        public float[] data;

        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 3)]
        ///<summary> Variance of noise for X, Y, and Z axis </summary>             
        public float[] noise_variances;

        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 3)]
        ///<summary> Variance of bias for X, Y, and Z axis </summary>              
        public float[] bias_variances;
    }
}
