// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.
#pragma once

#include <string>
#include "TrackingManager.h"

namespace librealsense
{
    template<size_t SIZE>
    std::string buffer_to_string(const uint8_t(&buff)[SIZE], char separator = ',', bool as_hex = false)
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

    //TODO: Use json library for class
    class controller_event_serializer 
    {
    public:
        static std::string serialized_data(const perc::TrackingData::ControllerDiscoveryEventFrame& frame)
        {
            std::string serizlied_data = to_string() 
                << "\"MAC\" : [" << buffer_to_string(frame.macAddress) << "]";
            return to_json(event_type_discovery, serizlied_data);
        }

        static std::string serialized_data(const perc::TrackingData::ControllerDisconnectedEventFrame& frame)
        {
            std::string serizlied_data = to_string()
                << "\"ID\" : " << (int)frame.controllerId;
            return to_json(event_type_disconnection, serizlied_data);
        }

        static std::string serialized_data(const perc::TrackingData::ControllerFrame& frame)
        {
            std::string serizlied_data = to_string() <<
                "\"sensorIndex\": " << (int)frame.sensorIndex << ","
                "\"frameId\": " << (int)frame.frameId << ","
                "\"eventId\": " << (int)frame.eventId << ","
                "\"instanceId\": " << (int)frame.instanceId << ","
                "\"sensorData\": [" << buffer_to_string(frame.sensorData) << "]";
            return to_json(event_type_frame, serizlied_data);
        }

        static std::string serialized_data(const perc::TrackingData::ControllerDeviceConnect& c, uint8_t controller_id)
        {
            std::string serizlied_data = to_string() 
                << "\"MAC\" : [" << buffer_to_string(c.macAddress) << "] ,"
                   "\"ID\" : " << (int)controller_id;
            return to_json(event_type_connection, serizlied_data);
        }
    private:
        static constexpr const char* prefix = R"JSON({"Event Type":"Controller Event", "Data" : {)JSON";
        static constexpr const char* suffix = "}}";
        static constexpr const char* event_type_frame = "Frame";
        static constexpr const char* event_type_connection = "Connection";
        static constexpr const char* event_type_disconnection = "Disconnection";
        static constexpr const char* event_type_discovery = "Discovery";

        static std::string to_json(const char* sub_type, const std::string& data)
        {
            return to_string()
                << prefix
                << "\"Sub Type\" : " << "\"" << sub_type << "\","
                << "\"Data\" : {" << data << "}"
                << suffix;
        }
    };
}