% Wraps librealsense2 motion_frame class
classdef motion_frame < realsense.frame
    methods
        % Constructor
        function this = motion_frame(handle)
            this = this@realsense.frame(handle);
        end

        % Destructor (uses base class destructor)
        
        % Functions
        function motion_data = get_motion_data(this)
            motion_data = realsense.librealsense_mex('rs2::motion_frame', 'get_motion_data', this.objectHandle);
        end
    end
end