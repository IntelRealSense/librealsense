classdef frame_metadata_value < uint64
    enumeration
        Frame_Counter       (0)
        Frame_Timestamp     (1)
        Sensor_Timestamp    (2)
        Actual_Exposure     (3)
        Gain_Level          (4)
        Auto_Exposure       (5)
        White_Balance       (6)
        Time_Of_Arrival     (7)
        Count               (8)
    end
end