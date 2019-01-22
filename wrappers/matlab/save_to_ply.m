% Wraps librealsense2 save_to_ply class
classdef save_to_ply < realsense.processing_block
    properties (Constant=true)
        option_ignore_color = realsense.option.count + 1;
    end
    methods
        % Constructor
        function this = save_to_ply(filename, pc)
            switch nargin
                case 0
                    out = realsense.librealsense_mex('rs2::save_to_ply', 'new');
                case 1
                    validateattributes(filename, {'char', 'string'}, {'scalartext'});
                    out = realsense.librealsense_mex('rs2::save_to_ply', 'new', filename);
                case 2
                    validateattributes(filename, {'char', 'string'}, {'scalartext'});
                    validateattributes(pc, {'realsense.pointcloud'}, {'scalar'});
                    out = realsense.librealsense_mex('rs2::save_to_ply', 'new', filename, pc.objectHandle);
            end
            this = this@realsense.processing_block(out);
        end
        
        % Destructor (uses base class destructor)
    end
end