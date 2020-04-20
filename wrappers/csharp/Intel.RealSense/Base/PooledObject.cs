// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense.Base
{
    using System;

    /// <summary>
    /// Base class for objects in an <cref see="ObjectPool">ObjectPool</cref>
    /// </summary>
    public abstract class PooledObject : Object
    {
        protected PooledObject(IntPtr ptr, Deleter deleter)
            : base(ptr, deleter)
        {
        }

        internal abstract void Initialize();

        protected override void Dispose(bool disposing)
        {
            if (m_instance.IsInvalid)
            {
                return;
            }

            base.Dispose(disposing);
            ObjectPool.Release(this);
        }
    }
}
