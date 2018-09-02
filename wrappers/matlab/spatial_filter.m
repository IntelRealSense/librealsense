% Wraps librealsense2 spatial_filter class
classdef spatial_filter < realsense.process_interface
    methods
        % Constructor
        function this = spatial_filter()
            out = realsense.librealsense_mex('rs2::spatial_filter', 'new');
            this = this@realsense.process_interface(out);
        end
        
        % Destructor (uses base class destructor)

        % Functions
    end
end