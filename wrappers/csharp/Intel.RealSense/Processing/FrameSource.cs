using System;
using System.Runtime.InteropServices;
using System.Collections.Generic;
using System.Linq;

namespace Intel.RealSense
{
    public struct FrameSource
    {
        internal readonly HandleRef m_instance;

        internal FrameSource(HandleRef instance)
        {
            m_instance = instance;
        }

        public T AllocateVideoFrame<T>(StreamProfile profile, Frame original, 
            int bpp, int width, int height, int stride, Extension extension = Extension.VideoFrame) where T : Frame
        {
            object error;
            var fref = NativeMethods.rs2_allocate_synthetic_video_frame(m_instance.Handle, profile.m_instance.Handle, original.m_instance.Handle, bpp, width, height, stride, extension, out error);
            return Frame.CreateFrame(fref) as T;
        }

        [Obsolete("This method is obsolete. Use AllocateCompositeFrame with DisposeWith method instead")]
        public FrameSet AllocateCompositeFrame(FramesReleaser releaser, params Frame[] frames)
        {
            return AllocateCompositeFrame((IList<Frame>)frames).DisposeWith(releaser);
        }

        public FrameSet AllocateCompositeFrame(params Frame[] frames)
        {
            return AllocateCompositeFrame((IList<Frame>)frames);
        }


        public FrameSet AllocateCompositeFrame(IList<Frame> frames)
        {
            if (frames == null)
                throw new ArgumentNullException(nameof(frames));

            IntPtr frame_refs = IntPtr.Zero;

            try {
                object error;
                int fl = frames.Count;
                frame_refs = Marshal.AllocHGlobal(fl * IntPtr.Size);
                for (int i = 0; i < fl; i++)
                {
                    var fr = frames[i].m_instance.Handle;
                    Marshal.WriteIntPtr(frame_refs, i * IntPtr.Size, fr);
                    NativeMethods.rs2_frame_add_ref(fr, out error);
                }

                var frame_ref = NativeMethods.rs2_allocate_composite_frame(m_instance.Handle, frame_refs, fl, out error);
                return FrameSet.Pool.Get(frame_ref);
            }
            finally
            {
                if (frame_refs != IntPtr.Zero)
                    Marshal.FreeHGlobal(frame_refs);
            }
        }

        private void FrameReady(IntPtr ptr)
        {
            object error;
            NativeMethods.rs2_frame_add_ref(ptr, out error);
            NativeMethods.rs2_synthetic_frame_ready(m_instance.Handle, ptr, out error);
        }

        public void FrameReady(Frame f)
        {
            FrameReady(f.m_instance.Handle);
        }

        public void FramesReady(FrameSet fs)
        {
            using (fs)
                FrameReady(fs.m_instance.Handle);
        }
    }
}