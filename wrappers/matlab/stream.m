classdef stream < int64
    enumeration
        any_stream  ( 0)
        depth       ( 1)
        color       ( 2)
        infrared    ( 3)
        fisheye     ( 4)
        gyro        ( 5)
        accel       ( 6)
        gpio        ( 7)
        pose        ( 8)
        confidence  ( 9)
        count       (10)
    end
end