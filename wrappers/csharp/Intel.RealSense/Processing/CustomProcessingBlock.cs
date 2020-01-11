// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Runtime.InteropServices;

    /// <summary>
    /// Custom processing block based on a delegate. This lets the users pass frames between module boundaries for processing
    /// </summary>
    // TODO: extends Base.Object, ctor is trickly...
    public class CustomProcessingBlock : IOptions, IDisposable
    {
        internal Base.DeleterHandle m_instance;

        private static readonly frame_processor_callback fpc = new frame_processor_callback(ProcessingBlockCallback);
        private static readonly frame_callback frameCallback = new frame_callback(ProcessingBlockFrameCallback);
        private readonly FrameQueue queue = new FrameQueue(1);
        private readonly GCHandle frameProcessorCallbackHandle;
        private GCHandle frameCallbackHandle;

        /// <summary>
        /// Initializes a new instance of the <see cref="CustomProcessingBlock"/> class.
        /// </summary>
        /// <param name="cb">Processing function to be applied to every frame entering the block</param>
        public CustomProcessingBlock(FrameProcessorCallback cb)
        {
            frameProcessorCallbackHandle = GCHandle.Alloc(cb, GCHandleType.Normal);
            IntPtr cbPtr = GCHandle.ToIntPtr(frameProcessorCallbackHandle);
            object error;
            var pb = NativeMethods.rs2_create_processing_block_fptr(fpc, cbPtr, out error);
            m_instance = new Base.DeleterHandle(pb, NativeMethods.rs2_delete_processing_block);
            Options = new OptionsList(m_instance.Handle);
        }

        ~CustomProcessingBlock()
        {
            Dispose(false);
        }

        private static void ProcessingBlockCallback(IntPtr f, IntPtr src, IntPtr u)
        {
            try
            {
                var callback = GCHandle.FromIntPtr(u).Target as FrameProcessorCallback;
                using (var frame = Frame.Create(f))
                {
                    callback(frame, new FrameSource(new HandleRef(frame, src)));
                }
            }
            catch (ArgumentException)
            {
                // ArgumentException: GCHandle value belongs to a different domain
                // Happens when Unity Editor stop the scene with multiple devices streaming with multiple post-processing filters.
            }
        }

        /// <inheritdoc/>
        public IOptionsContainer Options { get; private set; }

        public FrameQueue Queue
        {
            get
            {
                return queue;
            }
        }

        /// <summary>
        /// This method is used to pass frames into a processing block
        /// </summary>
        /// <typeparam name="T"><see cref="Frame"/> type or subclass</typeparam>
        /// <param name="f">Frame to process</param>
        public void Process<T>(T f)
            where T : Frame
        {
            object error;
            NativeMethods.rs2_frame_add_ref(f.Handle, out error);
            NativeMethods.rs2_process_frame(m_instance.Handle, f.Handle, out error);
        }

        /// <summary>
        /// Start the processing block, delivering frames to external queue
        /// </summary>
        /// <param name="queue"><see cref="FrameQueue"/> to place the processed frames in</param>
        public void Start(FrameQueue queue)
        {
            object error;
            NativeMethods.rs2_start_processing_queue(m_instance.Handle, queue.Handle, out error);
        }

        /// <summary>
        /// Start the processing block
        /// </summary>
        public void Start()
        {
            object error;
            NativeMethods.rs2_start_processing_queue(m_instance.Handle, Queue.Handle, out error);
        }

        /// <summary>
        /// Start the processing block, delivering frames to a callback
        /// </summary>
        /// <param name="cb">callback to receive frames</param>
        // TODO: overload with state object and Action<Frame, object> callback to avoid allocations
        public void Start(FrameCallback cb)
        {
            frameCallbackHandle = GCHandle.Alloc(cb, GCHandleType.Normal);
            IntPtr cbPtr = GCHandle.ToIntPtr(frameCallbackHandle);

            object error;
            NativeMethods.rs2_start_processing_fptr(m_instance.Handle, frameCallback, cbPtr, out error);
        }

        /// <summary>
        /// This method adds a custom option to a custom processing block.
        /// </summary>
        /// <param name="option_id">ID for referencing the option</param>
        /// <param name="min">the minimum value which will be accepted for this option</param>
        /// <param name="max">the maximum value which will be accepted for this option</param>
        /// <param name="step">the granularity of options which accept discrete values, or zero if the option accepts continuous values</param>
        /// <param name="def">the default value of the option.This will be the initial value.</param>
        /// <returns>true if adding the option succeeds. false if it fails e.g. an option with this id is already registered</returns>
        public bool RegisterOption(Option option_id, float min, float max, float step, float def)
        {
            object error;
            return NativeMethods.rs2_processing_block_register_simple_option(m_instance.Handle, option_id, min, max, step, def, out error) > 0;
        }

        private static void ProcessingBlockFrameCallback(IntPtr f, IntPtr u)
        {
            try
            {
                var callback = GCHandle.FromIntPtr(u).Target as FrameCallback;
                using (var frame = Frame.Create(f))
                {
                    callback(frame);
                }
            }
            catch (ArgumentException)
            {
                // ArgumentException: GCHandle value belongs to a different domain
                // Happens when Unity Editor stop the scene with multiple devices streaming with multiple post-processing filters.
            }
        }

        protected virtual void Dispose(bool disposing)
        {
            if (frameCallbackHandle.IsAllocated)
            {
                frameCallbackHandle.Free();
            }

            if (frameProcessorCallbackHandle.IsAllocated)
            {
                frameProcessorCallbackHandle.Free();
            }

            m_instance.Dispose();
        }

        /// <inheritdoc/>
        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        /// <summary>
        /// Processing delegate to be applied to every frame entering the block
        /// </summary>
        /// <param name="frame">frame entering the processing block</param>
        /// <param name="source">frame source used to notify frame ready <see cref="FrameSource.FrameReady(Frame)"/> </param>
        public delegate void FrameProcessorCallback(Frame frame, FrameSource source);
    }
}