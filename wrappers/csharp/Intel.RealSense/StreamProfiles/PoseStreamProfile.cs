// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Runtime.InteropServices;
    using System.Text;

    public class PoseStreamProfile : StreamProfile
    {
        public PoseStreamProfile(IntPtr ptr)
            : base(ptr)
        {
        }
    }
}
