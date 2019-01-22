using Intel.RealSense.Frames;
using Intel.RealSense.Processing;
using System;
using System.Collections.Generic;
using System.Text;

namespace Intel.RealSense.Frames
{
    public static class FrameExtension
    {
        public static FrameSet AsFrameSet(this Frame frame)
            => FrameSet.FromFrame(frame);

        public static Frame ApplyFilter(this Frame frame, IProcessingBlock block) 
            => block.Process(frame);
    }
}
