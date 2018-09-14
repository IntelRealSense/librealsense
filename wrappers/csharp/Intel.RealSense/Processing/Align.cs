using Intel.RealSense.Types;
using System.Runtime.InteropServices;

namespace Intel.RealSense.Processing
{
    public class Align : ProcessingBlock
    {
        private readonly FrameQueue queue;

        public Align(Stream alignTo)
        {
            queue = new FrameQueue(1);
            Instance = new HandleRef(this, NativeMethods.rs2_create_align(alignTo, out var error));
            NativeMethods.rs2_start_processing_queue(Instance.Handle, queue.Instance.Handle, out error);
        }

        public FrameSet Process(FrameSet original, FramesReleaser releaser = null)
        {
            NativeMethods.rs2_frame_add_ref(original.Instance.Handle, out var error);
            NativeMethods.rs2_process_frame(Instance.Handle, original.Instance.Handle, out error);
            return FramesReleaser.ScopedReturn(releaser, queue.WaitForFrames() as FrameSet);
        }
    }
}