using Intel.RealSense.Frames;
using Intel.RealSense.Types;
using System;
using System.Runtime.InteropServices;

namespace Intel.RealSense.Processing
{
    public class CustomProcessingBlock : ProcessingBlock
    {
        //public delegate void FrameCallback<Frame, T>(Frame frame, T user_data);
        public delegate void FrameCallback(Frame frame);
        public delegate void FrameProcessorCallback(Frame frame, FrameSource source);

        private FrameCallbackHandler Callback;
        private readonly FrameProcessorCallbackHandler procCallback;
        private FrameQueue Queue;

        public CustomProcessingBlock(FrameProcessorCallback cb)
        {
            void cb2(IntPtr f, IntPtr src, IntPtr u)
            {
                using (var frame = new Frame(f))
                    cb(frame, new FrameSource(new HandleRef(this, src)));
            }

            procCallback = cb2;
            Instance = new HandleRef(this, NativeMethods.rs2_create_processing_block_fptr(cb2, IntPtr.Zero, out var error));
        }

        public void ProcessFrame(Frame f)
        {
            NativeMethods.rs2_frame_add_ref(f.Instance.Handle, out var error);
            NativeMethods.rs2_process_frame(Instance.Handle, f.Instance.Handle, out error);
        }

        public void ProcessFrames(FrameSet fs)
        {
            using (var f = fs.AsFrame())
                ProcessFrame(f);
        }

        public void Start(FrameQueue queue)
        {
            NativeMethods.rs2_start_processing_queue(Instance.Handle, queue.Instance.Handle, out var error);

            Callback = null;
            Queue = queue;
        }

        public void Start(FrameCallback cb)
        {
            void cb2(IntPtr f, IntPtr u)
            {
                using (var frame = new Frame(f))
                    cb(frame);
            }
            NativeMethods.rs2_start_processing_fptr(Instance.Handle, cb2, IntPtr.Zero, out var error);
            Callback = cb2;
            Queue = null;
        }
    }
}