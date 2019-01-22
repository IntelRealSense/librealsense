using Intel.RealSense.Frames;
using Intel.RealSense.StreamProfiles;
using Intel.RealSense.Types;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;

namespace Intel.RealSense.Processing
{
    public struct FrameSource
    {
        internal readonly HandleRef Instance;

        internal FrameSource(HandleRef instance)
        {
            Instance = instance;
        }

        public T AllocateVideoFrame<T>(StreamProfile profile, Frame original,
           int bpp, int width, int height, int stride, Extension extension = Extension.VideoFrame) where T : Frame
        {
            var fref = NativeMethods.rs2_allocate_synthetic_video_frame(Instance.Handle, profile.Instance.Handle, original.Instance.Handle, bpp, width, height, stride, extension, out var error);
            return Frame.CreateFrame(fref) as T;
        }

        [Obsolete("This method is obsolete. Use AllocateCompositeFrame with DisposeWith method instead")]
        public FrameSet AllocateCompositeFrame(FramesReleaser releaser, params Frame[] frames)
            => AllocateCompositeFrame((IList<Frame>)frames).DisposeWith(releaser);

        public FrameSet AllocateCompositeFrame(params Frame[] frames)
            => AllocateCompositeFrame((IList<Frame>)frames);

        public FrameSet AllocateCompositeFrame(IList<Frame> frames)
        {
            if (frames == null)
                throw new ArgumentNullException(nameof(frames));

            IntPtr frameRefs = IntPtr.Zero;

            try
            {
                object error;
                int fl = frames.Count;
                frameRefs = Marshal.AllocHGlobal(fl * IntPtr.Size);
                for (int i = 0; i < fl; i++)
                {
                    var fr = frames[i].Instance.Handle;
                    Marshal.WriteIntPtr(frameRefs, i * IntPtr.Size, fr);
                    NativeMethods.rs2_frame_add_ref(fr, out error);
                }

                var frame_ref = NativeMethods.rs2_allocate_composite_frame(Instance.Handle, frameRefs, fl, out error);
                return FrameSet.Pool.Get(frame_ref);
            }
            finally
            {
                if (frameRefs != IntPtr.Zero)
                    Marshal.FreeHGlobal(frameRefs);
            }
        }

        public void FrameReady(IntPtr ptr)
        {
            NativeMethods.rs2_frame_add_ref(ptr, out var error);
            NativeMethods.rs2_synthetic_frame_ready(Instance.Handle, ptr, out error);
        }
        public void FrameReady(Frame frame) 
            => FrameReady(frame.Instance.Handle);

        public void FramesReady(FrameSet frameSet)
        {
            using (frameSet)
                FrameReady(frameSet.Instance.Handle);
        }
    }
}