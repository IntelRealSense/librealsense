using Intel.RealSense.Processing;
using System;
using System.Collections.Generic;
using System.Text;

namespace Intel.RealSense.Frames
{
    public static class FrameSetExtension
    {
        public static FrameSet ApplyFilter(this FrameSet frames, IProcessingBlock block) 
            => block.Process(frames);
    }
}
