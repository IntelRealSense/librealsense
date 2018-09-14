using Intel.RealSense.Frames;
using System.Runtime.InteropServices;

namespace Intel.RealSense.Processing
{
    public class HoleFillingFilter : ProcessingBlock
    {
        private readonly FrameQueue queue;

        public HoleFillingFilter()
        {
            queue = new FrameQueue(1);
            Instance = new HandleRef(this, NativeMethods.rs2_create_hole_filling_filter_block(out var error));
            NativeMethods.rs2_start_processing_queue(Instance.Handle, queue.Instance.Handle, out error);
        }

        public VideoFrame ApplyFilter(VideoFrame original)
        {
            NativeMethods.rs2_frame_add_ref(original.Instance.Handle, out var error);
            NativeMethods.rs2_process_frame(Instance.Handle, original.Instance.Handle, out error);
            return queue.WaitForFrame() as VideoFrame;
        }


    }
}