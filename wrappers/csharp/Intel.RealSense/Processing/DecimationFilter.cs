using Intel.RealSense.Frames;
using System.Runtime.InteropServices;

namespace Intel.RealSense.Processing
{
    public class DecimationFilter : ProcessingBlock
    {
        private readonly FrameQueue queue;

        public DecimationFilter()
        {
            queue = new FrameQueue(1);
            Instance = new HandleRef(this, NativeMethods.rs2_create_decimation_filter_block(out var error));
            NativeMethods.rs2_start_processing_queue(Instance.Handle, queue.Instance.Handle, out error);
        }

        public VideoFrame ApplyFilter(VideoFrame original, FramesReleaser releaser = null)
        {
            NativeMethods.rs2_frame_add_ref(original.Instance.Handle, out var error);
            NativeMethods.rs2_process_frame(Instance.Handle, original.Instance.Handle, out error);
            return FramesReleaser.ScopedReturn(releaser, queue.WaitForFrame() as VideoFrame);
        }

    }
}