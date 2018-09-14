using Intel.RealSense.Frames;
using Intel.RealSense.Profiles;
using Intel.RealSense.Types;
using System.Linq;
using System.Runtime.InteropServices;

namespace Intel.RealSense.Processing
{
    public class FrameSource
    {
        internal HandleRef Instance;

        internal FrameSource(HandleRef instance)
        {
            Instance = instance;
        }

        public VideoFrame AllocateVideoFrame(StreamProfile profile, Frame original, int bpp, int width, int height, int stride, Extension extension = Extension.VideoFrame)
        {
            var fref = NativeMethods.rs2_allocate_synthetic_video_frame(Instance.Handle, profile.Instance.Handle, original.Instance.Handle, bpp, width, height, stride, extension, out var error);
            return new VideoFrame(fref);
        }

        public FrameSet AllocateCompositeFrame(FramesReleaser releaser, params Frame[] frames)
        {
            object error;
            var frameRefs = frames.Select(x => x.Instance.Handle).ToArray();
            foreach (var fref in frameRefs) NativeMethods.rs2_frame_add_ref(fref, out error);
            var frameRef = NativeMethods.rs2_allocate_composite_frame(Instance.Handle, frameRefs, frames.Count(), out error);
            return FramesReleaser.ScopedReturn(releaser, new FrameSet(frameRef));
        }

        public FrameSet AllocateCompositeFrame(params Frame[] frames)
            => AllocateCompositeFrame(null, frames);

        public void FrameReady(Frame f)
        {
            NativeMethods.rs2_frame_add_ref(f.Instance.Handle, out var error);
            NativeMethods.rs2_synthetic_frame_ready(Instance.Handle, f.Instance.Handle, out error);
        }

        public void FramesReady(FrameSet fs)
        {
            using (var f = fs.AsFrame())
                FrameReady(f);
        }
    }
}