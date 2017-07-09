// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

/** 
* \file status.h
* @brief Describes the return status codes used by SDK interfaces.
*/

#pragma once

namespace rs
{
    namespace file_format
    {
        /** @brief
         *  Defines return codes that SDK interfaces
         *  use.  Negative values indicate errors, a zero value indicates success,
         *  and positive values indicate warnings.
         */
        enum status
        {
            /* success */
            status_no_error = 0,                /**< Operation succeeded without any warning */

            /* errors */
            status_feature_unsupported = -1,    /**< Unsupported feature */
            status_param_unsupported = -2,      /**< Unsupported parameter(s) */
            status_item_unavailable = -3,       /**< Item not found/not available */
            status_key_already_exists = -4,     /**< Key already exists in the data structure */
            status_invalid_argument = -5,       /**< Argument passed to the method is invalid */
            status_allocation_failled = -6,     /**< Failure in allocation */

            status_file_write_failed = -401,    /**< Failure in open file in WRITE mode */
            status_file_read_failed = -402,     /**< Failure in open file in READ mode */
            status_file_close_failed = -403,    /**< Failure in close a file handle */
            status_file_eof = -404,             /**< EOF */

        };
    }
}

