% Wraps librealsense2 pose_frame class
classdef pose_frame < realsense.frame
    methods
        % Constructor
        function this = pose_frame(handle)
            this = this@realsense.frame(handle);
        end

        % Destructor (uses base class destructor)
        
        % Functions
        function pose_data = get_pose_data(this)
            pose_data = realsense.librealsense_mex('rs2::pose_frame', 'get_pose_data', this.objectHandle);
        end
    end
end