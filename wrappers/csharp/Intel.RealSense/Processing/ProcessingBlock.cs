using System;
using System.Runtime.InteropServices;
using System.Collections.Generic;
using System.Linq;
using Intel.RealSense.Base;

namespace Intel.RealSense
{
    public interface IProcessingBlock : IOptions
    {
        Frame Process(Frame original);
    }

    public abstract class ProcessingBlock : Base.Object, IProcessingBlock
    {
        public readonly FrameQueue queue = new FrameQueue(1);

        public IOptionsContainer Options { get; private set; }

        protected ProcessingBlock(IntPtr ptr) : base(ptr, NativeMethods.rs2_delete_processing_block)
        {
            Options = new Sensor.SensorOptions(Handle);
        }

        /// <summary>
        /// Process frame and return the result
        /// </summary>
        /// <param name="original">Frame to process</param>
        /// <returns>Processed frame</returns>
        public Frame Process(Frame original)
        {
            return Process<Frame>(original);
        }

        /// <summary>
        /// Process frame and return the result
        /// </summary>
        /// <typeparam name="T">Type of frame to return</typeparam>
        /// <param name="original">Frame to process</param>
        /// <returns>Processed frame</returns>
        public T Process<T>(Frame original) where T : Frame
        {
            object error;
            NativeMethods.rs2_frame_add_ref(original.Handle, out error);
            NativeMethods.rs2_process_frame(Handle, original.Handle, out error);
            T f;
            if (queue.PollForFrame<T>(out f))
            {
                return f;
            }
            // this occurs when the sdk runs out of frame resources and allocate_video_frame fails
            // sadly, that exception doesn't propagate here...
            // usually a sign of not properly disposing of frames
            throw new InvalidOperationException($"Error while running {GetType().Name} processing block, check the log for info");
        }

        protected override void Dispose(bool disposing)
        {
            queue.Dispose();
            base.Dispose(disposing);
        }
    }

    public static class IProcessingBlockExtensions
    {
        public static Frame ApplyFilter(this Frame frame, IProcessingBlock block)
        {
            return block.Process(frame);
        }
    }
}