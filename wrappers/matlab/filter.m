% Wraps librealsense2 filter class
classdef filter < realsense.options
    methods
        % Constructor
        function this = filter(handle)
            this = this@realsense.options(handle);
        end
        
        % Destructor (uses base class destructor)

        % Functions
        function out_frame = process(this, frame)
            narginchk(2, 2)
            validateattributes(frame, {'realsense.frame'}, {'scalar'}, '', 'frame', 2);
            out = realsense.librealsense_mex('rs2::filter', 'process', this.objectHandle, frame.objectHandle);
            out_frame = realsense.frame(out);
        end
    end
end