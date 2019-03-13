using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;

namespace Intel.RealSense
{
    public class FrameQueue : Base.Object
    {
        internal static IntPtr Create(int capacity = 1)
        {
            object error;
            return NativeMethods.rs2_create_frame_queue(capacity, out error);
        }

        public FrameQueue(int capacity = 1) : base(Create(capacity), NativeMethods.rs2_delete_frame_queue)
        {
            Capacity = capacity;
        }

        public readonly int Capacity;
         
        public bool PollForFrame<T>(out T frame) where T : Frame
        {
            object error;
            IntPtr ptr;
            if (NativeMethods.rs2_poll_for_frame(Handle, out ptr, out error) > 0)
            {
                frame = Frame.Create<T>(ptr);
                return true;
            }
            frame = null;
            return false;
        }

        public Frame WaitForFrame(uint timeout_ms = 5000u)
        {
            return WaitForFrame<Frame>(timeout_ms);
        }

        public T WaitForFrame<T>(uint timeout_ms = 5000u) where T : Frame
        {
            object error;
            var ptr = NativeMethods.rs2_wait_for_frame(Handle, timeout_ms, out error);
            return Frame.Create<T>(ptr);
        }

        public bool TryWaitForFrame<T>(out T frame, uint timeout_ms = 5000u) where T : Frame
        {
            object error;
            IntPtr ptr;
            bool res = NativeMethods.rs2_try_wait_for_frame(Handle, timeout_ms, out ptr, out error) > 0;
            frame = res ? Frame.Create<T>(ptr) : null;
            return res;
        }

        public FrameSet WaitForFrames(uint timeout_ms = 5000u)
        {
            object error;
            var ptr = NativeMethods.rs2_wait_for_frame(Handle, timeout_ms, out error);
            return FrameSet.Create(ptr);
        }

        public void Enqueue(Frame f)
        {
            object error;
            NativeMethods.rs2_frame_add_ref(f.Handle, out error);
            NativeMethods.rs2_enqueue_frame(f.Handle, Handle);
        }
    }
}
