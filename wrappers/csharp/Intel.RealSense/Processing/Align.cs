using Intel.RealSense.Types;
using System;
using System.Runtime.InteropServices;

namespace Intel.RealSense.Processing
{
    public class Align : ProcessingBlock
    {
        public Align(Stream alignTo)
        {
            Instance = new HandleRef(this, NativeMethods.rs2_create_align(alignTo, out var error));
            NativeMethods.rs2_start_processing_queue(Instance.Handle, queue.Instance.Handle, out error);
        }

        [Obsolete("This method is obsolete. Use Process with DisposeWith method instead")]
        public FrameSet Process(FrameSet original, FramesReleaser releaser = null) 
            => Process(original).DisposeWith(releaser);
    }
}