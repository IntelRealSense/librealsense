% Wraps librealsense2 frame_queue class
classdef frame_queue < handle
    properties (SetAccess = private, Hidden = true)
        objectHandle;
    end
    methods
        % Constructor
        function this = frame_queue(capacity)
            if nargin == 0
                this.objectHandle = realsense.librealsense_mex('rs2::frame_queue', 'new');
            else
                validateattributes(capacity, {'numeric'}, {'scalar', 'positive', 'real', 'integer'});
                this.objectHandle = realsense.librealsense_mex('rs2::frame_queue', 'new', uint64(capacity));
            end
        end
        
        % Destructor
        function delete(this)
            if (this.objectHandle ~= 0)
                realsense.librealsense_mex('rs2::frame_queue', 'delete', this.objectHandle);
            end
        end
        
        % Functions
        function frame = wait_for_frame(this, timeout_ms)
            narginchk(1, 2);
            if nargin == 1
                out = realsense.librealsense_mex('rs2::frame_queue', 'wait_for_frame', this.objectHandle);
            else
                if isduration(timeout_ms)
                    timeout_ms = milliseconds(timeout_ms);
                end
                validateattributes(timeout_ms, {'numeric'}, {'scalar', 'nonnegative', 'real', 'integer'}, '', 'timeout_ms', 2);
                out = realsense.librealsense_mex('rs2::frame_queue', 'wait_for_frame', this.objectHandle, double(timeout_ms));
            end
            frame = realsense.frame(out);
        end
        % TODO: poll_for_frame [frame, video_frame, points, depth_frame, disparity_frame, motion_frame, pose_frame, frameset]
        function cap = capacity(this)
            cap = realsense.librealsense_mex('rs2::frame_queue', 'capacity', this.objectHandle);
        end
    end
end