// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Text;

    public interface IOption
    {
        Option Key { get; }

        /// <summary>Gets or sets option value</summary>
        /// <value>value of the option</value>
        float Value { get; set; }

        /// <summary>Gets the minimum value which will be accepted for this option</summary>
        float Min { get; }

        /// <summary>Gets the maximum value which will be accepted for this option</summary>
        float Max { get; }

        /// <summary>Gets the granularity of options which accept discrete values, or zero if the option accepts continuous values</summary>
        float Step { get; }

        /// <summary>Gets the default value of the option</summary>
        float Default { get; }

        /// <summary>Gets a value indicating whether an option is read-only</summary>
        /// <value><see langword="true"/> if option is read-only</value>
        bool ReadOnly { get; }

        /// <summary>Gets the option description</summary>
        /// <value>human-readable option description</value>
        string Description { get; }

        /// <summary>Gets the option value description (in case specific option value hold special meaning)</summary>
        /// <value>human-readable description of a specific value of an option or null if no special meaning</value>
        string ValueDescription { get; }
    }
}
