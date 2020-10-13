% Wraps librealsense2 align class
classdef align < realsense.filter
    methods
        % Constructor
        function this = align(align_to)
            narginchk(1, 1);
            validateattributes(align_to, {'realsense.stream', 'numeric'}, {'scalar', 'nonnegative', 'real', 'integer', '<=', int64(realsense.stream.count)});
            out = realsense.librealsense_mex('rs2::align', 'new', int64(align_to));
            this = this@realsense.filter(out);
        end
        
        % Destructor (uses base class destructor)
        
        % Functions
        function frames = process(this, frame)
            narginchk(2, 2);
            validateattributes(frame, {'realsense.frame'}, {'scalar'}, '', 'frame', 2);
            if ~frame.is('frameset')
                error('Expected input number 2, frame, to be a frameset');
            end
            out = realsense.librealsense_mex('rs2::align', 'process', this.objectHandle, frame.objectHandle);
            frames = realsense.frameset(out);
        end
    end
end
