// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.
#pragma once

#include <string>
#include "TrackingManager.h"

namespace librealsense
{
    template<size_t SIZE>
    inline std::string buffer_to_string(const uint8_t(&buff)[SIZE], char separator = ',', bool as_hex = false)
    {
        std::ostringstream oss;
        if (as_hex)
            oss << std::hex;

        for (size_t i = 0; i < SIZE; i++)
        {
            if (i != 0)
                oss << separator;
            oss << (int)buff[i];
        }
        return oss.str();
    }

    inline std::ostream& operator<<(std::ostream& os, const perc::TrackingData::Version& v)
    {
        return os << v.major << "." << v.minor << "." << v.patch << "." << v.build;
    }

    //TODO: Use json library for class
    class controller_event_serializer 
    {
    public:
        static std::string serialized_data(const perc::TrackingData::ControllerDiscoveryEventFrame& frame)
        {
            std::string serialized_data = to_string() 
                << "\"MAC\" : [" << buffer_to_string(frame.macAddress) << "]";
            return to_json(event_type_discovery(), serialized_data);
        }

        static std::string serialized_data(const perc::TrackingData::ControllerDisconnectedEventFrame& frame)
        {
            std::string serialized_data = to_string()
                << "\"ID\" : " << (int)frame.controllerId;
            return to_json(event_type_disconnection(), serialized_data);
        }

        static std::string serialized_data(const perc::TrackingData::ControllerFrame& frame)
        {
            std::string serialized_data = to_string() <<
                "\"sensorIndex\": " << (int)frame.sensorIndex << ","
                "\"frameId\": " << (int)frame.frameId << ","
                "\"eventId\": " << (int)frame.eventId << ","
                "\"instanceId\": " << (int)frame.instanceId << ","
                "\"sensorData\": [" << buffer_to_string(frame.sensorData) << "]";
            return to_json(event_type_frame(), serialized_data);
        }
        
        static std::string serialized_data(const perc::TrackingData::ControllerConnectedEventFrame& frame)
        {
            std::string serialized_data = to_string() <<
                "\"status\": \"" << (int)frame.status << "\","
                "\"controllerId\": " << (int)frame.controllerId << ","
                "\"manufacturerId\": " << (int)frame.manufacturerId << ","
                "\"protocol\": \"" << frame.protocol << "\","
                "\"app\": \"" << frame.app << "\","
                "\"softDevice\": \"" << frame.softDevice << "\","
                "\"bootLoader\": \"" << frame.bootLoader << "\"";
            return to_json(event_type_frame(), serialized_data);
        }
        
        static std::string serialized_data(const perc::TrackingData::ControllerDeviceConnect& c, uint8_t controller_id)
        {
            std::string serialized_data = to_string() 
                << "\"MAC\" : [" << buffer_to_string(c.macAddress) << "] ,"
                   "\"ID\" : " << (int)controller_id;
            return to_json(event_type_connection(), serialized_data);
        }
    private:
        static constexpr const char* prefix() { return R"JSON({"Event Type":"Controller Event", "Data" : {)JSON"; }
        static constexpr const char* suffix() { return "}}"; }
        static constexpr const char* event_type_frame() { return "Frame"; }
        static constexpr const char* event_type_connection() { return "Connection"; }
        static constexpr const char* event_type_disconnection() { return "Disconnection"; }
        static constexpr const char* event_type_discovery() { return "Discovery"; }

        static std::string to_json(const char* sub_type, const std::string& data)
        {
            return to_string()
                << prefix()
                << "\"Sub Type\" : " << "\"" << sub_type << "\","
                << "\"Data\" : {" << data << "}"
                << suffix();
        }
    };
}