% Wraps librealsense2 depth_sensor class
classdef depth_sensor < realsense.sensor
    methods
        % Constructor
        function this = depth_sensor(handle)
            this = this@realsense.sensor(handle);
        end
        
        % Destructor (uses base class destructor)

        % Functions
        function scale = get_depth_scale(this)
            scale = realsense.librealsense_mex('rs2::depth_sensor', 'get_depth_scale', this.objectHandle);
        end
    end
end