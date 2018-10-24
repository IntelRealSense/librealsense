using Intel.RealSense.Frames;
using System;
using System.Runtime.InteropServices;

namespace Intel.RealSense.Processing
{
    public class HoleFillingFilter : ProcessingBlock
    {
        public HoleFillingFilter()
        {
            Instance = new HandleRef(this, NativeMethods.rs2_create_hole_filling_filter_block(out var error));
            NativeMethods.rs2_start_processing_queue(Instance.Handle, queue.Instance.Handle, out error);
        }

        [Obsolete("This method is obsolete. Use Process method instead")]
        public VideoFrame ApplyFilter(VideoFrame original, FramesReleaser releaser = null)
            => Process(original).DisposeWith(releaser) as VideoFrame;
    }
}