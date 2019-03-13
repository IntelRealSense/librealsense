using System;
using System.Runtime.InteropServices;
using System.Collections.Generic;
using System.Linq;

namespace Intel.RealSense
{
    public class Align : ProcessingBlock
    {
        static IntPtr Create(Stream align_to)
        {
            object error;
            return NativeMethods.rs2_create_align(align_to, out error);
        }

        public Align(Stream align_to) : base(Create(align_to))
        {
            object error;
            NativeMethods.rs2_start_processing_queue(Handle, queue.Handle, out error);
        }

        [Obsolete("This method is obsolete. Use Process with DisposeWith method instead")]
        public FrameSet Process(FrameSet original, FramesReleaser releaser)
        {
            return Process(original).As<FrameSet>().DisposeWith(releaser);
        }
    }
}