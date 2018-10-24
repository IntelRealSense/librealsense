using Intel.RealSense.Frames;
using System;
using System.Runtime.InteropServices;

namespace Intel.RealSense.Processing
{
    public class Colorizer : ProcessingBlock
    {
        public Colorizer()
        {
            Instance = new HandleRef(this, NativeMethods.rs2_create_colorizer(out var error));
            NativeMethods.rs2_start_processing_queue(Instance.Handle, queue.Instance.Handle, out error);
        }

        [Obsolete("This method is obsolete. Use Process method instead")]
        public VideoFrame Colorize(VideoFrame original, FramesReleaser releaser = null)
            => Process(original).DisposeWith(releaser) as VideoFrame;

    }
}