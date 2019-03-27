// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Runtime.InteropServices;

    public class Colorizer : ProcessingBlock
    {
        private static IntPtr Create()
        {
            object error;
            return NativeMethods.rs2_create_colorizer(out error);
        }

        internal Colorizer(IntPtr ptr)
            : base(ptr)
        {
        }

        public Colorizer()
            : base(Create())
        {
            object error;
            NativeMethods.rs2_start_processing_queue(Handle, Queue.Handle, out error);
        }

        [Obsolete("This method is obsolete. Use Process method instead")]
        public VideoFrame Colorize(Frame original, FramesReleaser releaser = null)
        {
            return Process(original).As<VideoFrame>().DisposeWith(releaser);
        }
    }
}