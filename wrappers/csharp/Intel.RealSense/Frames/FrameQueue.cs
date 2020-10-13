// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Linq;
    using System.Runtime.InteropServices;
    using System.Text;

    public class FrameQueue : Base.Object
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="FrameQueue"/> class.
        /// <para>
        /// Frame queues are the simplest x-platform synchronization primitive provided by librealsense
        /// to help developers who are not using async APIs
        /// </para>
        /// </summary>
        /// <param name="capacity">max number of frames to allow to be stored in the queue before older frames will start to get dropped</param>
        public FrameQueue(int capacity = 1)
            : base(Create(capacity), NativeMethods.rs2_delete_frame_queue)
        {
            Capacity = capacity;
        }

        /// <summary>
        /// Gets the max number of frames to allow to be stored in the queue before older frames will start to get dropped
        /// </summary>
        public int Capacity { get; private set; }

        /// <summary>
        /// Poll if a new frame is available and dequeue if it is
        /// </summary>
        /// <typeparam name="T"><see cref="Frame"/> type or subclass</typeparam>
        /// <param name="frame">dequeued frame</param>
        /// <returns>true if new frame was stored to <paramref name="frame"/></returns>
        public bool PollForFrame<T>(out T frame)
            where T : Frame
        {
            object error;
            IntPtr ptr;
            if (NativeMethods.rs2_poll_for_frame(Handle, out ptr, out error) > 0)
            {
                frame = Frame.Create<T>(ptr);
                return true;
            }

            frame = null;
            return false;
        }

        /// <summary>
        /// Wait until new frame becomes available in the queue and dequeue it
        /// </summary>
        /// <param name="timeout_ms">max time in milliseconds to wait until an exception will be thrown</param>
        /// <returns>dequeued frame</returns>
        public Frame WaitForFrame(uint timeout_ms = 5000u)
        {
            return WaitForFrame<Frame>(timeout_ms);
        }

        /// <summary>
        /// Wait until new frame becomes available in the queue and dequeue it
        /// </summary>
        /// <typeparam name="T"><see cref="Frame"/> type or subclass</typeparam>
        /// <param name="timeout_ms">max time in milliseconds to wait until an exception will be thrown</param>
        /// <returns>dequeued frame</returns>
        public T WaitForFrame<T>(uint timeout_ms = 5000u)
            where T : Frame
        {
            object error;
            var ptr = NativeMethods.rs2_wait_for_frame(Handle, timeout_ms, out error);
            return Frame.Create<T>(ptr);
        }

        /// <summary>
        /// Wait until new frame becomes available in the queue and dequeue it
        /// </summary>
        /// <typeparam name="T"><see cref="Frame"/> type or subclass</typeparam>
        /// <param name="frame">dequeued frame</param>
        /// <param name="timeout_ms">max time in milliseconds to wait until a frame becomes available</param>
        /// <returns>true if new frame was stored to <paramref name="frame"/></returns>
        public bool TryWaitForFrame<T>(out T frame, uint timeout_ms = 5000u)
            where T : Frame
        {
            object error;
            IntPtr ptr;
            bool res = NativeMethods.rs2_try_wait_for_frame(Handle, timeout_ms, out ptr, out error) > 0;
            frame = res ? Frame.Create<T>(ptr) : null;
            return res;
        }

        /// <summary>
        /// Wait until new frame becomes available in the queue and dequeue it
        /// </summary>
        /// <param name="timeout_ms">max time in milliseconds to wait until a frame becomes available</param>
        /// <returns>dequeued frame</returns>
        public FrameSet WaitForFrames(uint timeout_ms = 5000u)
        {
            object error;
            var ptr = NativeMethods.rs2_wait_for_frame(Handle, timeout_ms, out error);
            return FrameSet.Create(ptr);
        }

        /// <summary>
        /// Enqueue new frame into a queue
        /// </summary>
        /// <param name="f">frame to enqueue</param>
        public void Enqueue(Frame f)
        {
            object error;
            NativeMethods.rs2_frame_add_ref(f.Handle, out error);
            NativeMethods.rs2_enqueue_frame(f.Handle, Handle);
        }

        internal static IntPtr Create(int capacity = 1)
        {
            object error;
            return NativeMethods.rs2_create_frame_queue(capacity, out error);
        }
    }
}
