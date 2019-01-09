using System;
using System.Runtime.InteropServices;
using System.Collections.Generic;
using System.Linq;

namespace Intel.RealSense
{
    public class Syncer : ProcessingBlock
    {
        public Syncer()
        {
            object error;
            m_instance = new HandleRef(this, NativeMethods.rs2_create_sync_processing_block(out error));
            NativeMethods.rs2_start_processing_queue(m_instance.Handle, queue.m_instance.Handle, out error);
        }

        public void SubmitFrame(Frame f)
        {
            object error;
            NativeMethods.rs2_frame_add_ref(f.m_instance.Handle, out error);
            NativeMethods.rs2_process_frame(m_instance.Handle, f.m_instance.Handle, out error);
        }

        public FrameSet WaitForFrames(uint timeout_ms = 5000)
        {
            object error;
            var ptr = NativeMethods.rs2_wait_for_frame(queue.m_instance.Handle, timeout_ms, out error);
            return FrameSet.Pool.Get(ptr);
        }

        public bool PollForFrames(out FrameSet result)
        {
            object error;
            IntPtr ptr;
            if (NativeMethods.rs2_poll_for_frame(queue.m_instance.Handle, out ptr, out error) > 0)
            {
                result = FrameSet.Pool.Get(ptr);
                return true;
            }
            result = null;
            return false;
        }
    }
}