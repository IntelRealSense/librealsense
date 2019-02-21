using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;

namespace Intel.RealSense
{
    public class PoseStreamProfile : StreamProfile
    {
        //internal static readonly ProfilePool<PoseStreamProfile> Pool = new ProfilePool<PoseStreamProfile>(ptr => new PoseStreamProfile(ptr));

        public PoseStreamProfile(IntPtr ptr) : base(ptr)
        {
        }

        //public override void Release()
        //{
        //    m_instance = new HandleRef(this, IntPtr.Zero);
        //    Pool.Release(this);
        //}
    }
}
