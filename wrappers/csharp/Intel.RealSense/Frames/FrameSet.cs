// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Runtime.InteropServices;

    public class FrameSet : Frame, ICompositeDisposable, IEnumerable<Frame>
    {
        private readonly List<IDisposable> disposables = new List<IDisposable>();

        [DebuggerBrowsable(DebuggerBrowsableState.Never)]
        private readonly Enumerator enumerator;

        [DebuggerBrowsable(DebuggerBrowsableState.Never)]
        private int count;

        /// <summary>
        /// Create a new <see cref="FrameSet"/> from <see cref="Frame"/>
        /// </summary>
        /// <param name="composite">a composite frame</param>
        /// <returns>a new <see cref="FrameSet"/> to be disposed</returns>
        /// <exception cref="ArgumentException">Thrown when frame is not a composite frame</exception>
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

        internal override void Initialize()
        {
            object error;
            count = NativeMethods.rs2_embedded_frames_count(Handle, out error);
            enumerator.Reset();
            disposables.Clear();
        }

        internal FrameSet(IntPtr ptr)
            : base(ptr)
        {
            object error;
            count = NativeMethods.rs2_embedded_frames_count(Handle, out error);
            enumerator = new Enumerator(this);
        }

        [DebuggerBrowsable(DebuggerBrowsableState.Never)]
        public new bool IsComposite => true;

        /// <summary>
        /// Cast this to a <see cref="Frame"/>
        /// </summary>
        /// <returns>a frame to be disposed</returns>
        public Frame AsFrame()
        {
            object error;
            NativeMethods.rs2_frame_add_ref(Handle, out error);
            return Frame.Create(Handle);
        }

        /// <summary>
        /// Invoke the <paramref name="action"/> delegate on each frame in the set
        /// </summary>
        /// <param name="action">Delegate to invoke</param>
        public void ForEach(Action<Frame> action)
        {
            for (int i = 0; i < count; i++)
            {
                using (var frame = this[i])
                {
                    action(frame);
                }
            }
        }

        public T FirstOrDefault<T>(Stream stream, Format format = Format.Any)
            where T : Frame
        {
            return FirstOrDefault(stream, format)?.Cast<T>();
        }

        public Frame FirstOrDefault(Stream stream, Format format = Format.Any)
        {
            for (int i = 0; i < count; i++)
            {
                var frame = this[i];
                using (var fp = frame.Profile)
                    if (fp.Stream == stream && (format == Format.Any || fp.Format == format))
                    {
                        return frame;
                    }

                frame.Dispose();
            }

            return null;
        }

        public T FirstOrDefault<T>(Predicate<T> predicate)
            where T : Frame
        {
            object error;
            for (int i = 0; i < count; i++)
            {
                var ptr = NativeMethods.rs2_extract_frame(Handle, i, out error);
                var frame = Frame.Create<T>(ptr);
                if (predicate(frame))
                {
                    return frame;
                }

                frame.Dispose();
            }

            return null;
        }

        /// <summary>
        /// Retrieve back the first frame of specific stream type, if no frame found, error will be thrown
        /// </summary>
        /// <typeparam name="T"><see cref="Frame"/> type or subclass</typeparam>
        /// <param name="stream">stream type of frame to be retrieved</param>
        /// <param name="format">format type of frame to be retrieved, defaults to <see cref="Format.Any"/></param>
        /// <returns>first found frame with <paramref name="stream"/> type and <paramref name="format"/> type</returns>
        /// <exception cref="ArgumentException">Thrown when requested type not found</exception>
        public T First<T>(Stream stream, Format format = Format.Any)
            where T : Frame
        {
            var f = FirstOrDefault<T>(stream, format);
            if (f == null)
            {
                throw new ArgumentException("Frame of requested stream type was not found!");
            }

            return f;
        }

        /// <summary>
        /// Retrieve back the first frame of specific stream type, if no frame found, error will be thrown
        /// </summary>
        /// <param name="stream">stream type of frame to be retrieved</param>
        /// <param name="format">format type of frame to be retrieved, defaults to <see cref="Format.Any"/></param>
        /// <returns>first found frame with <paramref name="stream"/> type and <paramref name="format"/> type</returns>
        /// <exception cref="ArgumentException">Thrown when requested type not found</exception>
        /// <seealso cref="First{T}(Stream, Format)"/>
        public Frame First(Stream stream, Format format = Format.Any) => First<Frame>(stream, format);

        /// <summary>Gets the first depth frame</summary>
        public DepthFrame DepthFrame => FirstOrDefault<DepthFrame>(Stream.Depth, Format.Z16);

        /// <summary>Gets the first color frame</summary>
        public VideoFrame ColorFrame => FirstOrDefault<VideoFrame>(Stream.Color);

        /// <summary>Gets the first infrared frame</summary>
        public VideoFrame InfraredFrame => FirstOrDefault<VideoFrame>(Stream.Infrared);

        /// <summary>Gets the first fisheye frame</summary>
        public VideoFrame FishEyeFrame => FirstOrDefault<VideoFrame>(Stream.Fisheye);

        /// <summary>Gets the first pose frame</summary>
        public PoseFrame PoseFrame => FirstOrDefault<PoseFrame>(Stream.Pose);

        /// <inheritdoc/>
        public IEnumerator<Frame> GetEnumerator()
        {
            enumerator.Reset();
            return enumerator;
        }

        /// <inheritdoc/>
        IEnumerator IEnumerable.GetEnumerator()
        {
            enumerator.Reset();
            return enumerator;
        }

        /// <summary>Gets the number of frames embedded within a composite frame</summary>
        /// <value>Number of embedded frames</value>
        public int Count => count;

        /// <summary>Extract frame from within a composite frame</summary>
        /// <param name="index">Index of the frame to extract within the composite frame</param>
        /// <returns>returns reference to a frame existing within the composite frame</returns>
        /// <exception cref="ArgumentOutOfRangeException">Thrown when <paramref name="index"/> is out of range</exception>
        public Frame this[int index]
        {
            get
            {
                if (index < count)
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
                    {
                        return p.Stream == stream && p.Index == index;
                    }
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
                    {
                        return p.Stream == stream && p.Format == format && p.Index == index;
                    }
                });
            }
        }

        public static new FrameSet Create(IntPtr ptr)
        {
            return ObjectPool.Get<FrameSet>(ptr);
        }

        protected override void Dispose(bool disposing)
        {
            disposables.ForEach(d => d?.Dispose());
            disposables.Clear();
            base.Dispose(disposing);
        }

        /// <inheritdoc/>
        public void AddDisposable(IDisposable disposable)
        {
            if (disposable == this)
            {
                return;
            }

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
                    if (index == 0 || index == fs.count + 1)
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
                if ((uint)index < (uint)fs.count)
                {
                    object error;
                    var ptr = NativeMethods.rs2_extract_frame(fs.Handle, index, out error);
                    current = Frame.Create(ptr);
                    index++;
                    return true;
                }

                index = fs.count + 1;
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

    public static class FrameSetExtensions
    {
        public static FrameSet AsFrameSet(this Frame frame)
        {
            return FrameSet.FromFrame(frame);
        }
    }
}
