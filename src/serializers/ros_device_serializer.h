// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once

#include <core/serialization.h>
#include "ros_device_serializer_writer.h"
#include "ros_reader.h"

namespace librealsense
{
    class ros_device_serializer: public device_serializer
    {
    public:
        ros_device_serializer(const std::string& file) :
            m_writer([file]() { 
				return std::make_shared<ros_device_serializer_writer>(file); 
			} ),
            m_reader([file]() { 
				return std::make_shared<ros_reader>(file); 
			} )
        {
        }

        std::shared_ptr<writer> get_writer() override
        {
            return *m_writer;
        }
        std::shared_ptr<reader> get_reader() override
        {
            return *m_reader;;
        }

    private:
        lazy<std::shared_ptr<writer>> m_writer;
        lazy<std::shared_ptr<reader>> m_reader;
    };
}