// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Runtime.InteropServices;

    public class ThresholdFilter : ProcessingBlock
    {
        private static IntPtr Create()
        {
            object error;
            return NativeMethods.rs2_create_threshold(out error);
        }

        internal ThresholdFilter(IntPtr ptr)
            : base(ptr)
        {
        }

        public ThresholdFilter()
            : base(Create())
        {
        }

        [Obsolete("This method is obsolete. Use Process method instead")]
        public VideoFrame ApplyFilter(Frame original, FramesReleaser releaser = null)
        {
            return Process(original).DisposeWith(releaser) as VideoFrame;
        }
    }
}