% Wraps librealsense2 pointcloud class
classdef pointcloud < realsense.options
    methods
        % Constructor
        function this = pointcloud()
            out = realsense.librealsense_mex('rs2::pointcloud', 'new');
            this = this@realsense.options(out);
        end
        
        % Destructor (uses base class destructor)

        % Functions
        function points = calculate(this, depth)
            narginchk(2, 2)
            validateattributes(depth, {'realsense.frame'}, {'scalar'}, '', 'depth', 2);
            if ~depth.is('depth_frame')
                error('Expected input number 2, depth, to be a depth_frame');
            end
            out = realsense.librealsense_mex('rs2::pointcloud', 'calculate', this.objectHandle, depth.objectHandle);
            points = realsense.points(out);
        end
        function map_to(this, mapped)
            narginchk(2, 2)
            validateattributes(mapped, {'realsense.frame'}, {'scalar'}, '', 'mapped', 2);
            realsense.librealsense_mex('rs2::pointcloud', 'map_to', this.objectHandle, mapped.objectHandle);
        end
    end
end