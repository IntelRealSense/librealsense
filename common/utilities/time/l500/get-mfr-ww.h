// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include <string>
#include <rsutils/time/work-week.h>


namespace utilities {
namespace time {
namespace l500 {

rsutils::time::work_week get_manufacture_work_week( const std::string & serial );

}  // namespace l500
}  // namespace time
}  // namespace utilities
