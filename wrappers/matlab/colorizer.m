% Wraps librealsense2 colorizer class
classdef colorizer < realsense.options
    methods
        % Constructor
        function this = colorizer()
            out = realsense.librealsense_mex('rs2::colorizer', 'new');
            this = this@realsense.options(out);
        end
        
        % Destructor (uses base class destructor)

        % Functions
        function video_frame = colorize(this, depth)
            narginchk(2, 2)
            validateattributes(depth, {'realsense.frame'}, {'scalar'}, '', 'depth', 2);
            if ~depth.is('depth_frame')
                error('Expected input number 2, depth, to be a depth_frame');
            end
            out = realsense.librealsense_mex('rs2::colorizer', 'colorize', this.objectHandle, depth.objectHandle);
            video_frame = realsense.video_frame(out);
        end
    end
end