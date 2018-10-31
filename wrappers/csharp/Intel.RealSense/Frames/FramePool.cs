using System;
#if NET35
/*
 * ---------------------------------------------------------------------
 * This code block is only for legacy Unity (version < 2018.1) support.
 * ---------------------------------------------------------------------
 */
#else
using System.Collections.Concurrent;
#endif
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;

namespace Intel.RealSense.Frames
{
    internal class FramePool<T> where T : Frame
    {
#if NET35
        /*
         * ---------------------------------------------------------------------
         * This code block is only for legacy Unity (version < 2018.1) support.
         * ---------------------------------------------------------------------
         */
        private readonly Stack<T> stack;
        private readonly object stackLock;
#else
        private readonly ConcurrentStack<T> stack;
#endif
        private readonly Func<IntPtr, T> factory;

        public FramePool(Func<IntPtr, T> factory)
        {
#if NET35
            /*
             * ---------------------------------------------------------------------
             * This code block is only for legacy Unity (version < 2018.1) support.
             * ---------------------------------------------------------------------
             */
            stack = new Stack<T>();
            stackLock = new object();
#else
            stack = new ConcurrentStack<T>();
#endif

            this.factory = factory;
        }

#if NET35
        /*
         * ---------------------------------------------------------------------
         * This code block is only for legacy Unity (version < 2018.1) support.
         * ---------------------------------------------------------------------
         */
        public T Get(IntPtr ptr)
        {
            int count;

            lock (stackLock)
            {
                count = stack.Count;
            }

            if (count > 0)
            {
                T frame;
                lock (stackLock)
                {
                    frame = stack.Pop();
                }

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
        {
            lock (stackLock)
            {
                stack.Push(t);
            }
        }
#else
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
#endif

    }
}
