// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once
#ifndef LIBREALSENSE_DEVICE_FACTORY_H
#define LIBREALSENSE_DEVICE_FACTORY_H

//#include "uvc.h"
#include "rscore.hpp"

/*
Factory class - Creates device object based on the USB VID/PID types
*/
class device_factory
{
public:

	// std::shared_ptr<rs_device> make_r200_device(std::shared_ptr<uvc::device> device);
	void register_type(const M)
	std::shared_ptr<rs_device> create_device(std::shared_ptr<uvc::device> device);
private:			// Singleton
	device_factory();
	device_factory(const device_factory &);
	~device_factory();

};

#endif	// LIBREALSENSE_DEVICE_FACTORY_H
