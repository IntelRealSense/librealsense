// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Runtime.InteropServices;

    public class Align : ProcessingBlock
    {
        private static IntPtr Create(Stream align_to)
        {
            object error;
            return NativeMethods.rs2_create_align(align_to, out error);
        }

        public Align(Stream align_to)
            : base(Create(align_to))
        {
        }

        [Obsolete("This method is obsolete. Use Process with DisposeWith method instead")]
        public FrameSet Process(FrameSet original, FramesReleaser releaser)
        {
            return Process(original).As<FrameSet>().DisposeWith(releaser);
        }
    }
}