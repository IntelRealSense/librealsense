// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include <string>


namespace librealsense {
namespace algo {
namespace camera_age {
namespace l500 {

class manufacture_time
{
	unsigned man_year;
	unsigned man_ww;

public:
	manufacture_time(unsigned man_year, unsigned man_ww) : man_year(man_year), man_ww(man_ww){}

	unsigned get_manufacture_year() const;
	unsigned get_manufacture_work_week() const;
};

manufacture_time get_manufature_time(std::string& serial);

// Returns the number of work weeks since given time
unsigned get_work_weeks_since(const manufacture_time& start);

}  // namespace l500
}  // namespace camera_age
}  // namespace algo
}  // namespace librealsense
