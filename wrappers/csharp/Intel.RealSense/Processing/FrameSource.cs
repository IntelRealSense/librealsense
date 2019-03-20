// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Runtime.InteropServices;

    public struct FrameSource
    {
        internal readonly HandleRef m_instance;

        internal FrameSource(HandleRef instance)
        {
            m_instance = instance;
        }

        public T AllocateVideoFrame<T>(StreamProfile profile, Frame original, int bpp, int width, int height, int stride, Extension extension = Extension.VideoFrame)
            where T : Frame
        {
            object error;
            var fref = NativeMethods.rs2_allocate_synthetic_video_frame(m_instance.Handle, profile.Handle, original.Handle, bpp, width, height, stride, extension, out error);
            return Frame.Create<T>(fref);
        }

        [Obsolete("This method is obsolete. Use AllocateCompositeFrame with DisposeWith method instead")]
        public FrameSet AllocateCompositeFrame(FramesReleaser releaser, params Frame[] frames)
        {
            return AllocateCompositeFrame((IList<Frame>)frames).DisposeWith(releaser);
        }

        /// <summary>
        /// Allocate new composite frame, aggregating a set of existing frames
        /// </summary>
        /// <param name="frames">array of frames</param>
        /// <returns>a newly allocated frame</returns>
        public FrameSet AllocateCompositeFrame(params Frame[] frames)
        {
            return AllocateCompositeFrame((IList<Frame>)frames);
        }

        /// <summary>
        /// Allocate new composite frame, aggregating a set of existing frames
        /// </summary>
        /// <param name="frames">list of frames</param>
        /// <returns>a newly allocated frame</returns>
        /// <exception cref="ArgumentNullException">Thrown when <paramref name="frames"/> is null</exception>
        public FrameSet AllocateCompositeFrame(IList<Frame> frames)
        {
            if (frames == null)
            {
                throw new ArgumentNullException(nameof(frames));
            }

            IntPtr frame_refs = IntPtr.Zero;

            try
            {
                object error;
                int fl = frames.Count;
                frame_refs = Marshal.AllocHGlobal(fl * IntPtr.Size);
                for (int i = 0; i < fl; i++)
                {
                    var fr = frames[i].Handle;
                    Marshal.WriteIntPtr(frame_refs, i * IntPtr.Size, fr);
                    NativeMethods.rs2_frame_add_ref(fr, out error);
                }

                var frame_ref = NativeMethods.rs2_allocate_composite_frame(m_instance.Handle, frame_refs, fl, out error);
                return FrameSet.Create(frame_ref);
            }
            finally
            {
                if (frame_refs != IntPtr.Zero)
                {
                    Marshal.FreeHGlobal(frame_refs);
                }
            }
        }

        /// <summary>
        /// This method will dispatch frame callback on a frame
        /// </summary>
        /// <param name="f">frame to dispatch</param>
        public void FrameReady(Frame f)
        {
            object error;
            NativeMethods.rs2_frame_add_ref(f.Handle, out error);
            NativeMethods.rs2_synthetic_frame_ready(m_instance.Handle, f.Handle, out error);
        }
    }
}