% Wraps librealsense2 hole_filling_filter class
classdef hole_filling_filter < realsense.process_interface
    methods
        % Constructor
        function this = hole_filling_filter()
            out = realsense.librealsense_mex('rs2::hole_filling_filter', 'new');
            this = this@realsense.process_interface(out);
        end
        
        % Destructor (uses base class destructor)

        % Functions
    end
end