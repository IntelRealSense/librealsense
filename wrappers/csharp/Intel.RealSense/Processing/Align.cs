using System;
using System.Runtime.InteropServices;
using System.Collections.Generic;
using System.Linq;

namespace Intel.RealSense
{
    public class Align : ProcessingBlock
    {
        public Align(Stream align_to)
        {
            object error;
            m_instance = new HandleRef(this, NativeMethods.rs2_create_align(align_to, out error));
            NativeMethods.rs2_start_processing_queue(m_instance.Handle, queue.m_instance.Handle, out error);
        }

        [Obsolete("This method is obsolete. Use Process with DisposeWith method instead")]
        public FrameSet Process(FrameSet original, FramesReleaser releaser)
        {
            return Process(original).DisposeWith(releaser);
        }
    }
}