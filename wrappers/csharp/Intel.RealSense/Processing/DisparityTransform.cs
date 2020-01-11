// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Runtime.InteropServices;

    public class DisparityTransform : ProcessingBlock
    {
        private static IntPtr Create(bool transform_to_disparity)
        {
            object error;
            byte transform_direction = transform_to_disparity ? (byte)1 : (byte)0;
            return NativeMethods.rs2_create_disparity_transform_block(transform_direction, out error);
        }

        internal DisparityTransform(IntPtr ptr)
            : base(ptr)
        {
        }

        public DisparityTransform(bool transform_to_disparity = true)
            : base(Create(transform_to_disparity))
        {
        }

        [Obsolete("This method is obsolete. Use Process method instead")]
        public VideoFrame ApplyFilter(Frame original, FramesReleaser releaser = null)
        {
            return Process(original).As<VideoFrame>().DisposeWith(releaser);
        }
    }
}