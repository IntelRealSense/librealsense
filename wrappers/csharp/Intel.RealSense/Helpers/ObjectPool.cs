using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq.Expressions;
using System.Reflection;

namespace Intel.RealSense
{
    using PooledStack = System.Collections.Generic.Stack<Base.PooledObject>;
    using ObjectFactory = Func<IntPtr, object>;
    
    public static class ObjectPool
    {
        public class TypeComparer : IEqualityComparer<Type>
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

        static readonly Dictionary<Type, PooledStack> pools = new Dictionary<Type, PooledStack>(TypeComparer.Default);
        static Dictionary<Type, ObjectFactory> factories = new Dictionary<Type, ObjectFactory>(TypeComparer.Default);

        static PooledStack GetPool(Type t)
        {
            Stack<Base.PooledObject> s;
            if (pools.TryGetValue(t, out s))
                return s;
            lock ((pools as ICollection).SyncRoot)
            {
                return pools[t] = new PooledStack();
            }
        }

        public static object CreateInstance(Type t, IntPtr ptr)
        {
            Func<IntPtr, object> factory;
            if (factories.TryGetValue(t, out factory))
                return factory(ptr);

            var ctorinfo = t.GetConstructor(BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.CreateInstance,
                    null, new Type[] { typeof(IntPtr) }, null);
            var args = new ParameterExpression[] { Expression.Parameter(typeof(IntPtr), "ptr") };
            var lambda = Expression.Lambda<ObjectFactory>(Expression.New(ctorinfo, args), args).Compile();
            lock ((factories as ICollection).SyncRoot)
            {
                factories[t] = lambda;
            }
            return lambda(ptr);
        }

        static object Get(Type t, IntPtr ptr)
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

        public static T Get<T>(IntPtr ptr) where T : Base.PooledObject
        {
            if (ptr == IntPtr.Zero)
                throw new ArgumentNullException(nameof(ptr));
            return Get(typeof(T), ptr) as T;
        }

        public static void Release<T>(T obj) where T : Base.PooledObject
        {
            var stack = GetPool(obj.GetType());
            lock ((stack as ICollection).SyncRoot)
            {
                stack.Push(obj);
            }
        }

    }
}