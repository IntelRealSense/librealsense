using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;

namespace Intel.RealSense.Frames
{
    internal class FramePool<T> where T : Frame
    {
        private readonly ConcurrentStack<T> stack;
        private readonly Func<IntPtr, T> factory;

        public FramePool(Func<IntPtr, T> factory)
        {
            stack = new ConcurrentStack<T>();

            this.factory = factory;
        }

        public T Get(IntPtr ptr)
        {
            if (stack.TryPop(out T frame))
            {
                frame.Instance = new HandleRef(frame, ptr);
                frame.disposedValue = false;
                return frame;
            }
            else
            {
                return factory(ptr);
            }

        }

        public void Release(T t)
            => stack.Push(t);
    }
}
