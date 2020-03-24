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
        function [res, frame] = poll_for_frame(this, type)
            narginchk(2, 2);
            % C++ function validates contents of type
            validateattributes(type, {'char', 'string'}, {'scalartext'}, '', 'type', 2);
            out = realsense.librealsense_mex('rs2::frame_queue', 'poll_for_frame', this.objectHandle, type)
            switch type
            case 'frame'
                frame = realsense.frame(out);
            case 'video_frame'
                frame = realsense.video_frame(out);
            case 'points'
                frame = realsense.points(out);
            case 'depth_frame'
                frame = realsense.depth_frame(out);
            case 'disparity_frame'
                frame = realsense.disparity_frame(out);
            case 'motion_frame'
                frame = realsense.motion_frame(out);
            case 'pose_frame'
                frame = realsense.pose_frame(out);
            case 'frameset'
                frame = realsense.frameset(out);
            end
        end
        function cap = capacity(this)
            cap = realsense.librealsense_mex('rs2::frame_queue', 'capacity', this.objectHandle);
        end
		function keep = keep_frames(this)
            cap = realsense.librealsense_mex('rs2::frame_queue', 'keep_frames', this.objectHandle);
        end
    end
end
