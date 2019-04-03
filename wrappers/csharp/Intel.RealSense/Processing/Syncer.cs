// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Runtime.InteropServices;

    public class Syncer : ProcessingBlock
    {
        private static IntPtr Create()
        {
            object error;
            return NativeMethods.rs2_create_sync_processing_block(out error);
        }

        internal Syncer(IntPtr ptr)
            : base(ptr)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Syncer"/> class.
        /// <para>
        /// This block accepts arbitrary frames and outputs composite frames of best matches.
        /// Some frames may be released within the syncer if they are waiting for match for too long.
        /// Syncronization is done (mostly) based on timestamps so good hardware timestamps are a pre-condition.
        /// </para>
        /// </summary>
        public Syncer()
            : base(Create())
        {
        }

        /// <summary>
        /// This method is used to pass frame into a processing block
        /// </summary>
        /// <param name="f">frame to process</param>
        public void SubmitFrame(Frame f)
        {
            object error;
            NativeMethods.rs2_frame_add_ref(f.Handle, out error);
            NativeMethods.rs2_process_frame(Handle, f.Handle, out error);
        }

        /// <summary>
        /// Wait until new frame becomes available in the queue and dequeue it
        /// </summary>
        /// <param name="timeout_ms">max time in milliseconds to wait until an exception will be thrown</param>
        /// <returns>dequeued frame</returns>
        public FrameSet WaitForFrames(uint timeout_ms = 5000)
        {
            object error;
            var ptr = NativeMethods.rs2_wait_for_frame(Queue.Handle, timeout_ms, out error);
            return FrameSet.Create(ptr);
        }

        /// <summary>
        /// Poll if a new frame is available and dequeue if it is
        /// </summary>
        /// <param name="result">dequeued frame</param>
        /// <returns>true if new frame was stored to <paramref name="result"/></returns>
        public bool PollForFrames(out FrameSet result)
        {
            object error;
            IntPtr ptr;
            if (NativeMethods.rs2_poll_for_frame(Queue.Handle, out ptr, out error) > 0)
            {
                result = FrameSet.Create(ptr);
                return true;
            }

            result = null;
            return false;
        }
    }
}