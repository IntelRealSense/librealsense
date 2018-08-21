% Wraps librealsense2 process_interface class
classdef process_interface < realsense.options
    methods
        % Constructor
        function this = process_interface(handle)
            this = this@realsense.options(handle);
        end
        
        % Destructor (uses base class destructor)

        % Functions
        function value = process(this, frame)
            narginchk(2, 2)
            validateattributes(frame, {'realsense.frame'}, {'scalar'}, '', 'frame', 2);
            out = realsense.librealsense_mex('rs2::process_interface', 'process', this.objectHandle, frame.objectHandle);
            value = realsense.frame(out);
        end
    end
end