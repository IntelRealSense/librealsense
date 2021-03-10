% Wraps librealsense2 temporal_filter class
classdef temporal_filter < realsense.filter
    methods
        % Constructor
        function this = temporal_filter(smooth_alpha, smooth_delta, persistence_control)
            if (nargin == 0)
                out = realsense.librealsense_mex('rs2::temporal_filter', 'new');
            elseif (nargin == 3)
                validateattributes(smooth_alpha, {'numeric'}, {'scalar', 'real'});
                validateattributes(smooth_delta, {'numeric'}, {'scalar', 'real'});
                validateattributes(persistence_control, {'numeric'}, {'scalar', 'real', 'integer'});
                out = realsense.librealsense_mex('rs2::temporal_filter', 'new', double(smooth_alpha), double(smooth_delta), int64(persistence_control));
            else
                % TODO: Error out on bad arg count
            end
            this = this@realsense.filter(out);
        end
        
        % Destructor (uses base class destructor)

        % Functions
    end
end
