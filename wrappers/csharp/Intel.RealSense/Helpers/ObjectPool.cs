using System;
using System.Collections;
using System.Collections.Generic;
using System.Reflection;

namespace Intel.RealSense
{
    public class ObjectPool
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

        readonly Dictionary<Type, Stack> pools = new Dictionary<Type, Stack>(TypeComparer.Default);
        readonly object @lock = new object();
        readonly Action<object, IntPtr> init;

        public ObjectPool(Action<object, IntPtr> init)
        {
            this.init = init;
        }

        Stack GetPool(Type t)
        {
            Stack s;
            if (pools.TryGetValue(t, out s))
                return s;
            return pools[t] = new Stack();
        }

        private object CreateInstance(Type t, IntPtr ptr)
        {
            return Activator.CreateInstance(t,
                BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.CreateInstance,
                null,
                new object[] { ptr },
                null, null);
        }

        public object Get(Type t, IntPtr ptr)
        {
            lock (@lock)
            {
                var stack = GetPool(t);
                if (stack.Count == 0)
                    return CreateInstance(t, ptr);
                var f = stack.Pop();
                init?.Invoke(f, ptr);
                return f;
            }
        }

        public T Get<T>(IntPtr ptr) where T : class
        {
            return Get(typeof(T), ptr) as T;
        }

        public void Release(object obj)
        {
            lock (@lock)
            {
                GetPool(obj.GetType()).Push(obj);
            }
        }
        
    }
}