using System;
using System.Runtime.InteropServices;
using System.Collections.Generic;
using System.Linq;

namespace Intel.RealSense
{
    public class SpatialFilter : ProcessingBlock
    {
        public SpatialFilter()
        {
            object error;
            m_instance = new HandleRef(this, NativeMethods.rs2_create_spatial_filter_block(out error));
            NativeMethods.rs2_start_processing_queue(m_instance.Handle, queue.m_instance.Handle, out error);
        }

        [Obsolete("This method is obsolete. Use Process method instead")]
        public VideoFrame ApplyFilter(Frame original, FramesReleaser releaser = null)
        {
            return Process(original).DisposeWith(releaser) as VideoFrame;
        }
    }
}