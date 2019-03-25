% Wraps librealsense2 points class
classdef points < realsense.frame
    methods
        % Constructor
        function this = points(handle)
            this = this@realsense.frame(handle);
        end

        % Destructor (uses base class destructor)
        
        % Functions
        function vertices = get_vertices(this)
            vertices = realsense.librealsense_mex('rs2::points', 'get_vertices', this.objectHandle);
        end
        function export_to_ply(this, fname, texture)
            narginchk(3, 3)
            validateattributes(fname, {'char'}, {'scalartext', 'nonempty'}, '', 'fname', 2);
            validateattributes(texture, {'realsense.frame'}, {'scalar'}, '', 'texture', 3);
            if ~texture.is('video_frame')
                error('Expected input number 3, texture, to be a video_frame');
            end
            realsense.librealsense_mex('rs2::points', 'export_to_ply', this.objectHandle, fname, texture.objectHandle);
        end
        function texture_coordinates = get_texture_coordinates(this)
            texture_coordinates = realsense.librealsense_mex('rs2::points', 'get_texture_coordinates', this.objectHandle);
        end
        function s = point_count(this)
            realsense.librealsense_mex('rs2::points', 'size', this.objectHandle);
        end
    end
end
