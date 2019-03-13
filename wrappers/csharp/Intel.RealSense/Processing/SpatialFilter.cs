using System;
using System.Runtime.InteropServices;
using System.Collections.Generic;
using System.Linq;

namespace Intel.RealSense
{
    public class SpatialFilter : ProcessingBlock
    {
        static IntPtr Create()
        {
            object error;
            return NativeMethods.rs2_create_spatial_filter_block(out error);
        }

        public SpatialFilter() : base(Create())
        {
            object error;
            NativeMethods.rs2_start_processing_queue(Handle, queue.Handle, out error);
        }

        [Obsolete("This method is obsolete. Use Process method instead")]
        public VideoFrame ApplyFilter(Frame original, FramesReleaser releaser = null)
        {
            return Process(original).As<VideoFrame>().DisposeWith(releaser);
        }
    }
}