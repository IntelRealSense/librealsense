% Wraps librealsense2 spatial_filter class
classdef spatial_filter < realsense.filter
    methods
        % Constructor
        function this = spatial_filter(smooth_alpha, smooth_delta, magnitude, hole_fill)
            if (nargin == 0)
                out = realsense.librealsense_mex('rs2::spatial_filter', 'new');
            elseif (nargin == 4)
                validateattributes(smooth_alpha, {'numeric'}, {'scalar', 'real'});
                validateattributes(smooth_delta, {'numeric'}, {'scalar', 'real'});
                validateattributes(magnitude, {'numeric'}, {'scalar', 'real'});
                validateattributes(hole_fill, {'numeric'}, {'scalar', 'real'});
                out = realsense.librealsense_mex('rs2::spatial_filter', 'new', double(smooth_alpha), double(smooth_delta), double(magnitude), double(hole_fill));
            else
                % TODO: Error out on bad arg count
            end
            this = this@realsense.filter(out);
        end
        
        % Destructor (uses base class destructor)

        % Functions
    end
end
