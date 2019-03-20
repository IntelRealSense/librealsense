// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Runtime.InteropServices;
    using System.Text;

    public class MotionStreamProfile : StreamProfile
    {
        public MotionStreamProfile(IntPtr ptr)
            : base(ptr)
        {
        }

        public MotionDeviceIntrinsics GetIntrinsics()
        {
            object error;
            MotionDeviceIntrinsics intrinsics;
            NativeMethods.rs2_get_motion_intrinsics(Handle, out intrinsics, out error);
            return intrinsics;
        }
    }
}
