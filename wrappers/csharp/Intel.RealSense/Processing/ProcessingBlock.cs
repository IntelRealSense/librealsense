// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Linq;
    using System.Runtime.InteropServices;
    using Intel.RealSense.Base;
    using System.Reflection;

    public interface IProcessingBlock : IOptions
    {
        Frame Process(Frame original);
    }

    /// <summary>
    /// Base class for processing blocks
    /// </summary>
    public class ProcessingBlock : Base.Object, IProcessingBlock
    {
        [DebuggerBrowsable(DebuggerBrowsableState.Never)]
        private readonly FrameQueue queue = new FrameQueue(1);

        /// <inheritdoc/>
        public IOptionsContainer Options { get; private set; }

        public InfoCollection Info { get; private set; }

        /// <summary>
        /// Gets the internal queue
        /// </summary>
        public FrameQueue Queue
        {
            get
            {
                return queue;
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="ProcessingBlock"/> class.
        /// <para>
        /// Starts the processing block directing it's output to the <see cref="Queue"/>
        /// </para>
        /// </summary>
        /// <param name="ptr">native <c>rs2_processing_block*</c> pointer</param>
        public ProcessingBlock(IntPtr ptr)
            : base(ptr, NativeMethods.rs2_delete_processing_block)
        {
            object error;
            Options = new OptionsList(Handle);
            Info = new InfoCollection(NativeMethods.rs2_supports_processing_block_info, NativeMethods.rs2_get_processing_block_info, Handle);

            NativeMethods.rs2_start_processing_queue(Handle, queue.Handle, out error);
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
        /// <exception cref="InvalidOperationException">Thrown when errors occur during processing</exception>
        public T Process<T>(Frame original)
            where T : Frame
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
            (Options as OptionsList).Dispose();
            base.Dispose(disposing);
        }

        /// <summary>Test if the given processing block can be extended to the requested extension</summary>
        /// <param name="extension">The extension to which the processing block should be tested if it is extendable</param>
        /// <returns><see langword="true"/> iff the processing block can be extended to the given extension</returns>
        public bool Is(Extension extension)
        {
            object error;
            return NativeMethods.rs2_is_processing_block_extendable_to(Handle, extension, out error) > 0;
        }

        /// <summary>Cast to a strongly-typed <see cref="ProcessingBlock"/> subclass</summary>
        /// <typeparam name="T"><see cref="ProcessingBlock"/> type or subclass</typeparam>
        /// <returns>an instance of <typeparamref name="T"/></returns>
        public T As<T>()
            where T : ProcessingBlock
        {
            return (T)Activator.CreateInstance(
                typeof(T),
                BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.CreateInstance,
                null,
                new object[] { Handle },
                null);
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