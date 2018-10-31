% Wraps librealsense2 align class
classdef align < librealsense.processing_block
    properties (SetAccess = private, Hidden = true)
        objectHandle;
    end
    methods
        % Constructor
        function this = align(align_to)
            narginchk(1, 1);
            validateattributes(align_to, {'realsense.stream', 'numeric'}, {'scalar', 'nonnegative', 'real', 'integer', '<=', realsense.stream.count});
            this.objectHandle = realsense.librealsense_mex('rs2::align', 'new', int64(align_to));
            this = this@realsense.processing_block(out);
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
