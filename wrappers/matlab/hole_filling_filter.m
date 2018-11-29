% Wraps librealsense2 hole_filling_filter class
classdef hole_filling_filter < realsense.filter
    methods
        % Constructor
        function this = hole_filling_filter(mode)
            if (nargin == 0)
                out = realsense.librealsense_mex('rs2::hole_filling_filter', 'new');
            else
                validateattributes(mode, {'numeric'}, {'scalar', 'real', 'integer'});
                out = realsense.librealsense_mex('rs2::hole_filling_filter', 'new', int64(mode));
            end
            this = this@realsense.filter(out);
        end
        
        % Destructor (uses base class destructor)

        % Functions
    end
end