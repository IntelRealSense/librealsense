% Wraps librealsense2 disparity_transform class
classdef disparity_transform < realsense.filter
    methods
        % Constructor
        function this = disparity_transform(transform_to_disparity)
            if nargin == 0
                out = realsense.librealsense_mex('rs2::disparity_transform', 'new');
            else
                validateattributes(transform_to_disparity, {'logical', 'numeric'}, {'scalar', 'real'});
                out = realsense.librealsense_mex('rs2::disparity_transform', 'new', logical(transform_to_disparity));
            end
            this = this@realsense.filter(out);
        end
        
        % Destructor (uses base class destructor)

        % Functions
    end
end