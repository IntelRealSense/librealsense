% Wraps librealsense2 save_to_ply class
classdef save_single_frameset < realsense.filter
    methods
        % Constructor
        function this = save_single_frameset(filename)
            switch nargin
                case 0
                    out = realsense.librealsense_mex('rs2::save_single_frameset', 'new');
                case 1
                    validateattributes(filename, {'char', 'string'}, {'scalartext'});
                    out = realsense.librealsense_mex('rs2::save_single_frameset', 'new', filename);
            end
            this = this@realsense.filter(out);
        end
        
        % Destructor (uses base class destructor)
    end
end