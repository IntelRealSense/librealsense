using Intel.RealSense.Frames;
using System;
using System.Runtime.InteropServices;

namespace Intel.RealSense.Processing
{
    public class Syncer : ProcessingBlock
    {
        public Syncer()
        {
            Instance = new HandleRef(this, NativeMethods.rs2_create_sync_processing_block(out var error));
            NativeMethods.rs2_start_processing_queue(Instance.Handle, queue.Instance.Handle, out error);
        }

        public void SubmitFrame(Frame frame)
        {
            NativeMethods.rs2_frame_add_ref(frame.Instance.Handle, out var error);
            NativeMethods.rs2_process_frame(Instance.Handle, frame.Instance.Handle, out error);
        }

        public FrameSet WaitForFrames(uint timeoutMs = 5000) 
            => FrameSet.Pool.Get(NativeMethods.rs2_wait_for_frame(queue.Instance.Handle, timeoutMs, out var error));

        public bool PollForFrames(out FrameSet result, FramesReleaser releaser = null)
        {
            result = null;

            if (NativeMethods.rs2_poll_for_frame(queue.Instance.Handle, out IntPtr ptr, out var error) > 0)
            {
                result = FrameSet.Pool.Get(ptr);
                return true;
            }

            return false;
        }
    }
}