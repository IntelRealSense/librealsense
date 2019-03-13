using System;
using System.Runtime.InteropServices;
using System.Collections.Generic;
using System.Linq;

namespace Intel.RealSense
{
    public class Syncer : ProcessingBlock
    {
        static IntPtr Create()
        {
            object error;
            return NativeMethods.rs2_create_sync_processing_block(out error);
        }

        public Syncer() : base(Create())
        {
            object error;
            NativeMethods.rs2_start_processing_queue(Handle, queue.Handle, out error);
        }

        public void SubmitFrame(Frame f)
        {
            object error;
            NativeMethods.rs2_frame_add_ref(f.Handle, out error);
            NativeMethods.rs2_process_frame(Handle, f.Handle, out error);
        }

        public FrameSet WaitForFrames(uint timeout_ms = 5000)
        {
            object error;
            var ptr = NativeMethods.rs2_wait_for_frame(queue.Handle, timeout_ms, out error);
            return FrameSet.Create(ptr);
        }

        public bool PollForFrames(out FrameSet result)
        {
            object error;
            IntPtr ptr;
            if (NativeMethods.rs2_poll_for_frame(queue.Handle, out ptr, out error) > 0)
            {
                result = FrameSet.Create(ptr);
                return true;
            }
            result = null;
            return false;
        }
    }
}