% Wraps librealsense2 decimation_filter class
classdef decimation_filter < realsense.processing_block
    methods
        % Constructor
        function this = decimation_filter(magnitude)
            if (nargin == 0)
                out = realsense.librealsense_mex('rs2::decimation_filter', 'new');
            else
                validateattributes(magnitude, {'numeric'}, {'scalar', 'nonnegative', 'real'});
                out = realsense.librealsense_mex('rs2::decimation_filter', 'new', double(magnitude));
            end
            this = this@realsense.processing_block(out);
        end
        
        % Destructor (uses base class destructor)

        % Functions
    end
end