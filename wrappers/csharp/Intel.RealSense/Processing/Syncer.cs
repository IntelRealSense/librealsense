using Intel.RealSense.Frames;
using System.Runtime.InteropServices;

namespace Intel.RealSense.Processing
{
    public class Syncer : ProcessingBlock
    {
        private readonly FrameQueue queue;

        public Syncer()
        {
            queue = new FrameQueue(1);
            Instance = new HandleRef(this, NativeMethods.rs2_create_sync_processing_block(out var error));
            NativeMethods.rs2_start_processing_queue(Instance.Handle, queue.Instance.Handle, out error);
        }

        public void SubmitFrame(Frame f, FramesReleaser releaser = null)
        {
            NativeMethods.rs2_frame_add_ref(f.Instance.Handle, out var error);
            NativeMethods.rs2_process_frame(Instance.Handle, f.Instance.Handle, out error);
        }

        public FrameSet WaitForFrames(uint timeout_ms = 5000, FramesReleaser releaser = null)
        {
            var ptr = NativeMethods.rs2_wait_for_frame(queue.Instance.Handle, timeout_ms, out var error);
            return FramesReleaser.ScopedReturn(releaser, new FrameSet(ptr));
        }

        public bool PollForFrames(out FrameSet result, FramesReleaser releaser = null)
        {
            if (NativeMethods.rs2_poll_for_frame(queue.Instance.Handle, out Frame f, out var error) > 0)
            {
                result = FramesReleaser.ScopedReturn(releaser, new FrameSet(f.Instance.Handle));
                f.Dispose();
                return true;
            }
            result = null;
            return false;
        }
    }
}