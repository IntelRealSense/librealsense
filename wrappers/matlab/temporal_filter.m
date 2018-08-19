% Wraps librealsense2 temporal_filter class
classdef temporal_filter < realsense.process_interface
    methods
        % Constructor
        function this = temporal_filter()
            out = realsense.librealsense_mex('rs2::temporal_filter', 'new');
            this = this@realsense.process_interface(out);
        end
        
        % Destructor (uses base class destructor)

        % Functions
    end
end