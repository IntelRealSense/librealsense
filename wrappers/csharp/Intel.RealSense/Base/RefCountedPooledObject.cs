// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

//Adapted from: https://github.com/dotnet/reactive/blob/master/Rx.NET/Source/src/System.Reactive/Disposables/RefCountDisposable.cs
namespace Intel.RealSense.Base
{
    using System;
    using System.Threading;

    /// <summary>
    /// Base class for objects in an <cref see="ObjectPool">ObjectPool</cref>
    /// </summary>
    public class RefCountedPooledObject : PooledObject
    {
        private RefCount refCount;

        protected RefCountedPooledObject(IntPtr ptr, Deleter deleter, RefCount _refCount)
            : base(ptr, deleter)
        {
            refCount = _refCount;
        }

        internal void Retain()
        {
            if (m_instance.IsInvalid)
            {
                throw new ObjectDisposedException("RefCountedPooledObject");
            }

            var cnt = Thread.VolatileRead(ref refCount.count);

            for (; ;)
            {
                if (cnt == int.MaxValue)
                {
                    throw new OverflowException("RefCountedPooledObject can't handle more than " + int.MaxValue + " disposables");
                }

                var u = Interlocked.CompareExchange(ref refCount.count, cnt + 1, cnt);
                if (u == cnt)
                {
                    break;
                }
                cnt = u;
            }
        }

        protected override void Dispose(bool disposing)
        {
            if (m_instance.IsInvalid)
            {
                return;
            }

            bool didDispose = Release(disposing);

            //Dispose of this instance even if the underlying resource still exists
            if (!didDispose)
            {
                m_instance.SetHandleAsInvalid();
                ObjectPool.Release(this);
            }
        }

        private bool Release(bool disposing)
        {
            bool didDispose = false;
            var cnt = Thread.VolatileRead(ref refCount.count);
            for (; ; )
            {
                System.Diagnostics.Debug.Assert(cnt > 0);

                var u = cnt - 1;
                var b = Interlocked.CompareExchange(ref refCount.count, u, cnt);
                if (b == cnt)
                {
                    if (u == 0)
                    {
                        base.Dispose(disposing);
                        didDispose = true;
                    }
                    break;
                }
                cnt = b;
            }
            return didDispose;
        }

        internal override void Initialize()
        {
            throw new NotImplementedException();
        }
    }

    public sealed class RefCount
    {
        public int count;
    }
}
