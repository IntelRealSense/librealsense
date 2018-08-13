% Wraps librealsense2 decimation_filter class
classdef decimation_filter < realsense.process_interface
    methods
        % Constructor
        function this = decimation_filter()
            out = realsense.librealsense_mex('rs2::decimation_filter', 'new');
            this = this@realsense.process_interface(out);
        end
        
        % Destructor (uses base class destructor)

        % Functions
    end
end