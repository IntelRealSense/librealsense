% Wraps librealsense2 disparity_frame class
classdef disparity_frame < realsense.depth_frame
    methods
        % Constructor
        function this = disparity_frame(handle)
            this = this@realsense.depth_frame(handle);
        end

        % Destructor (uses base class destructor)
        
        % Functions
        function baseline = get_baseline(this)
            baseline = realsense.librealsense_mex('rs2::disparity_frame', 'get_baseline', this.objectHandle);
        end
    end
end