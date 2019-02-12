using System;
using System.Runtime.InteropServices;
using System.Collections.Generic;
using System.Linq;

namespace Intel.RealSense
{
    public class CustomProcessingBlock : ProcessingBlock
    {
        public delegate void FrameProcessorCallback(Frame frame, FrameSource source);

        public CustomProcessingBlock(FrameProcessorCallback cb)
        {
            frameProcessorCallbackHandle = GCHandle.Alloc(cb, GCHandleType.Normal);
            IntPtr cbPtr = GCHandle.ToIntPtr(frameProcessorCallbackHandle);

            object error;
            var pb = NativeMethods.rs2_create_processing_block_fptr(fpc, cbPtr, out error);
            m_instance = new HandleRef(this, pb);
        }

        readonly frame_processor_callback fpc = new frame_processor_callback(ProcessingBlockCallback);

        static void ProcessingBlockCallback(IntPtr f, IntPtr src, IntPtr u)
        {
            try
            {
                var callback = GCHandle.FromIntPtr(u).Target as FrameProcessorCallback;
                using (var frame = Frame.CreateFrame(f))
                    callback(frame, new FrameSource(new HandleRef(frame, src)));
            }
            // ArgumentException: GCHandle value belongs to a different domain
            // Happens when Unity Editor stop the scene with multiple devices streaming with multiple post-processing filters.
            catch (ArgumentException) { }
        }

        public void ProcessFrame(Frame f)
        {
            object error;
            NativeMethods.rs2_frame_add_ref(f.m_instance.Handle, out error);
            NativeMethods.rs2_process_frame(m_instance.Handle, f.m_instance.Handle, out error);
        }

        public void ProcessFrames(FrameSet fs)
        {
            using (var f = fs.AsFrame())
                ProcessFrame(f);
        }

        /// <summary>
        /// Start the processing block, delivering frames to external queue
        /// </summary>
        /// <param name="queue"></param>
        public void Start(FrameQueue queue)
        {
            object error;
            NativeMethods.rs2_start_processing_queue(m_instance.Handle, queue.m_instance.Handle, out error);
        }

        /// <summary>
        /// Start the processing block
        /// </summary>
        public void Start()
        {
            object error;
            NativeMethods.rs2_start_processing_queue(m_instance.Handle, queue.m_instance.Handle, out error);
        }

        public delegate void FrameCallback(Frame frame);

        /// <summary>
        /// Start the processing block, delivering frames to a callback
        /// </summary>
        /// <param name="cb"></param>
        public void Start(FrameCallback cb)
        {
            frameCallbackHandle = GCHandle.Alloc(cb, GCHandleType.Normal);
            IntPtr cbPtr = GCHandle.ToIntPtr(frameCallbackHandle);

            object error;
            NativeMethods.rs2_start_processing_fptr(m_instance.Handle, m_frameCallback, cbPtr, out error);
        }

        readonly frame_callback m_frameCallback = new frame_callback(ProcessingBlockFrameCallback);
        static void ProcessingBlockFrameCallback(IntPtr f, IntPtr u)
        {
            try
            {
                var callback = GCHandle.FromIntPtr(u).Target as FrameCallback;
                using (var frame = Frame.CreateFrame(f))
                    callback(frame);
            }
            // ArgumentException: GCHandle value belongs to a different domain
            // Happens when Unity Editor stop the scene with multiple devices streaming with multiple post-processing filters.
            catch (ArgumentException) { }
        }

        private GCHandle frameCallbackHandle;
        private readonly GCHandle frameProcessorCallbackHandle;

        protected override void Dispose(bool disposing)
        {
            //if (!disposedValue)
            {
                if (disposing)
                {
                    if (frameCallbackHandle.IsAllocated)
                        frameCallbackHandle.Free();

                    if (frameProcessorCallbackHandle.IsAllocated)
                        frameProcessorCallbackHandle.Free();
                }
            }

            base.Dispose(disposing);
        }
    }
}