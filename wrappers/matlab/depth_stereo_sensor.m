% Wraps librealsense2 depth_stereo_sensor class
classdef depth_stereo_sensor < realsense.depth_sensor
    methods
        % Constructor
        function this = depth_stereo_sensor(handle)
            this = this@realsense.depth_sensor(handle);
        end
        
        % Destructor (uses base class destructor)

        % Functions
        function scale = get_stereo_baseline(this)
            scale = realsense.librealsense_mex('rs2::depth_stereo_sensor', 'get_stereo_baseline', this.objectHandle);
        end
    end
end