// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Linq.Expressions;
    using System.Reflection;
    using ObjectFactory = System.Func<System.IntPtr, object>;
    using PooledStack = System.Collections.Generic.Stack<Base.PooledObject>;

    /// <summary>
    /// Object pool to reuse objects, avoids allocation and GC pauses
    /// </summary>
    public static class ObjectPool
    {
        private static readonly Dictionary<Type, PooledStack> Pools = new Dictionary<Type, PooledStack>(TypeComparer.Default);
        private static readonly Dictionary<Type, ObjectFactory> Factories = new Dictionary<Type, ObjectFactory>(TypeComparer.Default);

        private class TypeComparer : IEqualityComparer<Type>
        {
            public static readonly TypeComparer Default = new TypeComparer();

            public bool Equals(Type x, Type y)
            {
                return x == y;
            }

            public int GetHashCode(Type obj)
            {
                return obj.GetHashCode();
            }
        }

        private static PooledStack GetPool(Type t)
        {
            Stack<Base.PooledObject> s;
            if (Pools.TryGetValue(t, out s))
            {
                return s;
            }

            lock ((Pools as ICollection).SyncRoot)
            {
                return Pools[t] = new PooledStack();
            }
        }

        private static object CreateInstance(Type t, IntPtr ptr)
        {
            Func<IntPtr, object> factory;
            if (Factories.TryGetValue(t, out factory))
            {
                return factory(ptr);
            }

            var ctorinfo = t.GetConstructor(
                    BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.CreateInstance,
                    null,
                    new Type[] { typeof(IntPtr) },
                    null);
            var args = new ParameterExpression[] { Expression.Parameter(typeof(IntPtr), "ptr") };
            var lambda = Expression.Lambda<ObjectFactory>(Expression.New(ctorinfo, args), args).Compile();
            lock ((Factories as ICollection).SyncRoot)
            {
                Factories[t] = lambda;
            }

            return lambda(ptr);
        }

        private static object Get(Type t, IntPtr ptr)
        {
            var stack = GetPool(t);
            int count;
            lock ((stack as ICollection).SyncRoot)
            {
                count = stack.Count;
            }

            if (count > 0)
            {
                Base.PooledObject obj;
                lock ((stack as ICollection).SyncRoot)
                {
                    obj = stack.Pop();
                }

                obj.m_instance.Reset(ptr);
                obj.Initialize();
                return obj;
            }

            return CreateInstance(t, ptr);
        }

        /// <summary>
        /// Get an object from the pool, should be released back
        /// </summary>
        /// <typeparam name="T">type of object</typeparam>
        /// <param name="ptr">native handle</param>
        /// <returns>an object of type <typeparamref name="T"/></returns>
        public static T Get<T>(IntPtr ptr)
            where T : Base.PooledObject
        {
            if (ptr == IntPtr.Zero)
            {
                throw new ArgumentNullException(nameof(ptr));
            }

            return Get(typeof(T), ptr) as T;
        }

        /// <summary>
        /// Return an object to the pool
        /// </summary>
        /// <typeparam name="T">type of object</typeparam>
        /// <param name="obj">object to return to pool</param>
        public static void Release<T>(T obj)
            where T : Base.PooledObject
        {
            var stack = GetPool(obj.GetType());
            lock ((stack as ICollection).SyncRoot)
            {
                stack.Push(obj);
            }
        }
    }
}