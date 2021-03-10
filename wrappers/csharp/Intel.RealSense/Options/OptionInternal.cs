// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    using System;
    using System.Diagnostics;
    using System.Runtime.InteropServices;

    [DebuggerDisplay("{Value}", Name = "{Key}")]
    internal sealed class OptionInternal : IOption
    {
        [DebuggerBrowsable(DebuggerBrowsableState.Never)]
        private readonly IntPtr m_options;

        [DebuggerBrowsable(DebuggerBrowsableState.Never)]
        private readonly Option option;

        [DebuggerBrowsable(DebuggerBrowsableState.Never)]
        private readonly float min;

        [DebuggerBrowsable(DebuggerBrowsableState.Never)]
        private readonly float max;

        [DebuggerBrowsable(DebuggerBrowsableState.Never)]
        private readonly float step;

        [DebuggerBrowsable(DebuggerBrowsableState.Never)]
        private readonly float @default;

        [DebuggerBrowsable(DebuggerBrowsableState.Never)]
        private string description;

        internal OptionInternal(IntPtr options, Option option)
        {
            m_options = options;
            this.option = option;

            object error;
            NativeMethods.rs2_get_option_range(m_options, option, out min, out max, out step, out @default, out error);
        }

        /// <summary>Gets the option description</summary>
        /// <value>human-readable option description</value>
        public string Description
        {
            get
            {
                if (description == null)
                {
                    object error;
                    var str = NativeMethods.rs2_get_option_description(m_options, option, out error);
                    description = Marshal.PtrToStringAnsi(str);
                }

                return description;
            }
        }

        /// <summary>Gets or sets option value</summary>
        /// <value>value of the option</value>
        public float Value
        {
            get
            {
                object error;
                return NativeMethods.rs2_get_option(m_options, option, out error);
            }

            set
            {
                object error;
                NativeMethods.rs2_set_option(m_options, option, value, out error);
            }
        }

        /// <summary>get option value description (in case specific option value hold special meaning)</summary>
        /// <param name="value">value of the option</param>
        /// <returns>human-readable description of a specific value of an option or null if no special meaning</returns>
        public string GetValueDescription(float value)
        {
            object error;
            var str = NativeMethods.rs2_get_option_value_description(m_options, option, value, out error);
            return Marshal.PtrToStringAnsi(str);
        }

        public Option Key => option;

        public float Min => min;

        public float Max => max;

        public float Step => step;

        public float Default => @default;

        /// <summary>Gets the option value description (in case specific option value hold special meaning)</summary>
        /// <value>human-readable description of a specific value of an option or null if no special meaning</value>
        public string ValueDescription => GetValueDescription(Value);

        /// <summary>Gets a value indicating whether an option is read-only</summary>
        /// <value><see langword="true"/> if option is read-only</value>
        public bool ReadOnly
        {
            get
            {
                object error;
                return NativeMethods.rs2_is_option_read_only(m_options, option, out error) != 0;
            }
        }
    }
}
