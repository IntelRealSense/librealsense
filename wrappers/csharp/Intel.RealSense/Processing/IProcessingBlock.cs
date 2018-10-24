using Intel.RealSense.Frames;
using Intel.RealSense.Types;
using System;
using System.Collections.Generic;
using System.Text;

namespace Intel.RealSense.Processing
{
    public interface IProcessingBlock : IOptions
    {
        Frame Process(Frame original);
        FrameSet Process(FrameSet original);
    }
}
