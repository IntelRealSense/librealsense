using System;
using System.Runtime.InteropServices;
using System.Collections.Generic;
using System.Linq;

namespace Intel.RealSense
{
    public class DisparityTransform : ProcessingBlock
    {
        public DisparityTransform(bool transform_to_disparity = true)
        {
            object error;
            byte transform_direction = transform_to_disparity ? (byte)1 : (byte)0;
            m_instance = new HandleRef(this, NativeMethods.rs2_create_disparity_transform_block(transform_direction, out error));
            NativeMethods.rs2_start_processing_queue(m_instance.Handle, queue.m_instance.Handle, out error);
        }

        [Obsolete("This method is obsolete. Use Process method instead")]
        public VideoFrame ApplyFilter(Frame original, FramesReleaser releaser = null)
        {
            return Process(original).DisposeWith(releaser) as VideoFrame;
        }
    }
}