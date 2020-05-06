% Wraps librealsense2 save_to_ply class
classdef save_to_ply < realsense.filter
    properties (Constant=true)
        option_ignore_color = realsense.option.count + 1;
        option_ply_mesh = realsense.option.count + 2;
        option_ply_binary = realsense.option.count + 3;
        option_ply_normals = realsense.option.count + 4;
        option_ply_treshold = realsense.option.count + 5;
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
            this = this@realsense.filter(out);
        end

        % Destructor (uses base class destructor)
    end
end
