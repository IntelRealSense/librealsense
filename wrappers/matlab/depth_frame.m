% Wraps librealsense2 depth_frame class
classdef depth_frame < realsense.video_frame
    methods
        % Constructor
        function this = depth_frame(handle)
            this = this@realsense.video_frame(handle);
        end

        % Destructor (uses base class destructor)
        
        % Functions
        function distance = get_distance(this, x, y)
            narginchk(3, 3);
            validateattributes(x, {'numeric'}, {'scalar', 'nonnegative', 'real', 'integer'}, '', 'x', 2);
            validateattributes(y, {'numeric'}, {'scalar', 'nonnegative', 'real', 'integer'}, '', 'y', 2);
            distance = realsense.librealsense_mex('rs2::depth_frame', 'get_distance', this.objectHandle, int64(x), int64(y));
        end
    end
end