using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    public class FrameSet : Frame, ICompositeDisposable, IEnumerable<Frame>
    {
        internal override void Initialize()
        {
            object error;
            m_count = NativeMethods.rs2_embedded_frames_count(Handle, out error);
            m_enum.Reset();
            disposables.Clear();
        }

        internal int m_count;
        internal readonly Enumerator m_enum;

        internal FrameSet(IntPtr ptr) : base(ptr)
        {
            object error;
            m_count = NativeMethods.rs2_embedded_frames_count(Handle, out error);
            m_enum = new Enumerator(this);
        }

        public Frame AsFrame()
        {
            object error;
            NativeMethods.rs2_frame_add_ref(Handle, out error);
            return Frame.Create(Handle);
        }

        public static FrameSet FromFrame(Frame composite)
        {
            if (composite.IsComposite)
            {
                object error;
                NativeMethods.rs2_frame_add_ref(composite.Handle, out error);
                return Create(composite.Handle);
            }
            throw new ArgumentException("The frame is a not composite frame", nameof(composite));
        }

        [Obsolete("This method is obsolete. Use DisposeWith method instead")]
        public static FrameSet FromFrame(Frame composite, FramesReleaser releaser)
        {
            return FromFrame(composite).DisposeWith(releaser);
        }

        public void ForEach(Action<Frame> action)
        {
            for (int i = 0; i < m_count; i++)
            {
                using (var frame = this[i])
                    action(frame);
            }
        }


        public T FirstOrDefault<T>(Stream stream, Format format = Format.Any) where T : Frame
        {
            //return FirstOrDefault(stream, format)?.As<T>();
            using (var f = FirstOrDefault(stream, format))
                return f?.As<T>();

            //var f = FirstOrDefault(stream, format);
            //if (f == null)
            //    return null;
            //object error;
            //NativeMethods.rs2_frame_add_ref(f.Handle, out error);
            //var r = Create<T>(f.Handle);
            //f.m_instance.SetHandleAsInvalid();
            //Pool.Release(f);
            //return r;
        }

        public Frame FirstOrDefault(Stream stream, Format format = Format.Any)
        {
            for (int i = 0; i < m_count; i++)
            {
                var frame = this[i];
                using (var fp = frame.Profile)
                    if (fp.Stream == stream && (format == Format.Any || fp.Format == format))
                        return frame;
                frame.Dispose();
            }
            return null;
        }

        public T FirstOrDefault<T>(Predicate<T> predicate) where T : Frame
        {
            object error;
            for (int i = 0; i < m_count; i++)
            {
                var ptr = NativeMethods.rs2_extract_frame(Handle, i, out error);
                var frame = Frame.Create<T>(ptr);
                if (predicate(frame))
                    return frame;
                frame.Dispose();
            }
            return null;
        }
        
        public T First<T>(Stream stream, Format format = Format.Any) where T : Frame
        {
            var f = FirstOrDefault<T>(stream, format);
            if (f == null)
                throw new ArgumentException("Frame of requested stream type was not found!");
            return f;
        }

        public Frame First(Stream stream, Format format = Format.Any)
        {
            return First<Frame>(stream, format);
        }

        public DepthFrame DepthFrame
        {
            get
            {
                return FirstOrDefault<DepthFrame>(Stream.Depth, Format.Z16);
            }
        }

        public VideoFrame ColorFrame
        {
            get
            {
                return FirstOrDefault<VideoFrame>(Stream.Color);
            }
        }

        public VideoFrame InfraredFrame
        {
            get
            {
                return FirstOrDefault<VideoFrame>(Stream.Infrared);
            }
        }

        public IEnumerator<Frame> GetEnumerator()
        {
            m_enum.Reset();
            return m_enum;
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            m_enum.Reset();
            return m_enum;
        }

        /// <summary>Get number of frames embedded within a composite frame</summary>
        /// <value>Number of embedded frames</value>
        public int Count
        {
            get
            {
                return m_count;
            }
        }

        /// <summary>Extract frame from within a composite frame</summary>
        /// <param name="index">Index of the frame to extract within the composite frame</param>
        /// <returns>returns reference to a frame existing within the composite frame</returns>
        public Frame this[int index]
        {
            get
            {
                if (index < m_count)
                {
                    object error;
                    var ptr = NativeMethods.rs2_extract_frame(Handle, index, out error);
                    return Frame.Create(ptr);
                }
                throw new ArgumentOutOfRangeException(nameof(index));
            }
        }

        public Frame this[Stream stream, int index = 0]
        {
            get
            {
                return FirstOrDefault<Frame>(f =>
                {
                    using (var p = f.Profile)
                        return p.Stream == stream && p.Index == index;
                });
            }
        }


        public Frame this[Stream stream, Format format, int index = 0]
        {
            get
            {
                return FirstOrDefault<Frame>(f =>
                {
                    using (var p = f.Profile)
                        return p.Stream == stream && p.Format == format && p.Index == index;
                });
            }
        }

        public static new FrameSet Create(IntPtr ptr) {
            return Pool.Get<FrameSet>(ptr);
        }

        protected override void Dispose(bool disposing)
        {
            disposables.ForEach(d => d?.Dispose());
            disposables.Clear();
            base.Dispose(disposing);
        }

        internal readonly List<IDisposable> disposables = new List<IDisposable>();
        public void AddDisposable(IDisposable disposable)
        {
            if (disposable == this)
                return;
            disposables.Add(disposable);
        }

        internal sealed class Enumerator : IEnumerator<Frame>
        {
            private readonly FrameSet fs;
            private Frame current;
            private int index;

            public Enumerator(FrameSet fs)
            {
                this.fs = fs;
                index = 0;
                current = default(Frame);
            }

            public Frame Current
            {
                get
                {
                    return current;
                }
            }

            object IEnumerator.Current
            {
                get
                {
                    if (index == 0 || index == fs.m_count + 1)
                    {
                        throw new InvalidOperationException();
                    }
                    return Current;
                }
            }

            public void Dispose()
            {
                // Method intentionally left empty.
            }

            public bool MoveNext()
            {
                if ((uint)index < (uint)fs.m_count)
                {
                    object error;
                    var ptr = NativeMethods.rs2_extract_frame(fs.Handle, index, out error);
                    current = Frame.Create(ptr);
                    index++;
                    return true;
                }
                index = fs.m_count + 1;
                current = null;
                return false;
            }

            public void Reset()
            {
                index = 0;
                current = null;
            }
        }
    }

    public static class FrameSetExtensions {
        public static FrameSet AsFrameSet(this Frame frame)
        {
            return FrameSet.FromFrame(frame);
        }
    }
}
