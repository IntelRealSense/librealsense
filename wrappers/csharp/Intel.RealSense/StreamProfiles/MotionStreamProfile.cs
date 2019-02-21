using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;

namespace Intel.RealSense
{
    public class MotionStreamProfile : StreamProfile
    {
        //internal static readonly ProfilePool<MotionStreamProfile> Pool = new ProfilePool<MotionStreamProfile>(ptr => new MotionStreamProfile(ptr));

        public MotionStreamProfile(IntPtr ptr) : base(ptr)
        {
        }

        public MotionDeviceIntrinsics GetIntrinsics()
        {
            object error;
            MotionDeviceIntrinsics intrinsics;
            NativeMethods.rs2_get_motion_intrinsics(m_instance.Handle, out intrinsics, out error);
            return intrinsics;
        }
        //public override void Release()
        //{
        //    m_instance = new HandleRef(this, IntPtr.Zero);
        //    ProfilePool<MotionStreamProfile>.Release(this);
        //}
    }
}
