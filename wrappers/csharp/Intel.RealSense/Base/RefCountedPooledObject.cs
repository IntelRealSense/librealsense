// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

//Adapted from: https://github.com/dotnet/reactive/blob/master/Rx.NET/Source/src/System.Reactive/Disposables/RefCountDisposable.cs
namespace Intel.RealSense.Base
{
    using System;

    /// <summary>
    /// Base class for objects in an <cref see="ObjectPool">ObjectPool</cref>
    /// </summary>
    public class RefCountedPooledObject : PooledObject
    {
        protected RefCount refCount;

        protected RefCountedPooledObject(IntPtr ptr, Deleter deleter)
            : base(ptr, deleter)
        {
        }

        internal void Retain()
        {
            if (m_instance.IsInvalid)
            {
                throw new ObjectDisposedException("RefCountedPooledObject");
            }

            if (refCount.count == int.MaxValue)
            {
                throw new OverflowException("RefCountedPooledObject can't handle more than " + int.MaxValue + " disposables");
            }
            refCount.count++;  
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
            System.Diagnostics.Debug.Assert(refCount.count > 0);

            refCount.count--;
            if (refCount.count == 0)
            {
                base.Dispose(disposing);
                return true;
            }

            return false;
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
