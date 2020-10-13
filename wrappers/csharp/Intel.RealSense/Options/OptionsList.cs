// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Diagnostics;
    using System.Runtime.InteropServices;

    [DebuggerTypeProxy(typeof(OptionsListDebugView))]
    [DebuggerDisplay("Count = {Count}")]
    internal sealed class OptionsList : Base.Object, IOptionsContainer, ICollection
    {
        internal sealed class OptionsListDebugView
        {
            private readonly OptionsList ol;

            [DebuggerBrowsable(DebuggerBrowsableState.RootHidden)]
            public IOption[] Items
            {
                get
                {
                    IOption[] array = new IOption[ol.Count];
                    ol.CopyTo(array, 0);
                    return array;
                }
            }

            public OptionsListDebugView(OptionsList optionList)
            {
                if (optionList == null)
                {
                    throw new ArgumentNullException(nameof(optionList));
                }

                ol = optionList;
            }
        }

        private readonly IntPtr options;

        /// <inheritdoc/>
        public int Count
        {
            get
            {
                object error;
                return NativeMethods.rs2_get_options_list_size(Handle, out error);
            }
        }

        /// <inheritdoc/>
        public object SyncRoot => this;

        /// <inheritdoc/>
        public bool IsSynchronized => false;

        internal static IntPtr Create(IntPtr options)
        {
            object error;
            return NativeMethods.rs2_get_options_list(options, out error);
        }

        internal OptionsList(IntPtr options)
            : base(Create(options), NativeMethods.rs2_delete_options_list)
        {
            this.options = options;
        }

        public IOption this[Option option]
        {
            get
            {
                return new OptionInternal(options, option);
            }
        }

        /// <summary>
        /// check if particular option is supported by a subdevice
        /// </summary>
        /// <param name="option">option id to be checked</param>
        /// <returns>true if option is supported</returns>
        public bool Supports(Option option)
        {
            object error;
            return NativeMethods.rs2_supports_option(options, option, out error) > 0;
        }

        /// <summary>get option value description (in case specific option value hold special meaning)</summary>
        /// <param name="option">option id to be checked</param>
        /// <param name="value">value of the option</param>
        /// <returns>human-readable description of a specific value of an option or null if no special meaning</returns>
        public string OptionValueDescription(Option option, float value)
        {
            object error;
            var desc = NativeMethods.rs2_get_option_value_description(options, option, value, out error);
            if (desc != IntPtr.Zero)
            {
                return Marshal.PtrToStringAnsi(desc);
            }

            return null;
        }

        /// <inheritdoc/>
        public IEnumerator<IOption> GetEnumerator()
        {
            object error;
            int size = NativeMethods.rs2_get_options_list_size(Handle, out error);
            for (int i = 0; i < size; i++)
            {
                var option = NativeMethods.rs2_get_option_from_list(Handle, i, out error);
                yield return new OptionInternal(options, option);
            }
        }

        /// <inheritdoc/>
        IEnumerator IEnumerable.GetEnumerator()
        {
            return GetEnumerator();
        }

        /// <inheritdoc/>
        public void CopyTo(Array array, int index)
        {
            if (array == null)
            {
                throw new ArgumentNullException("array");
            }

            object error;
            for (int i = 0; i < Count; i++)
            {
                var option = NativeMethods.rs2_get_option_from_list(Handle, i, out error);
                array.SetValue(this[option], i + index);
            }
        }
    }
}
