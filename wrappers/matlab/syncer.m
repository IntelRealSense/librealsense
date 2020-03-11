% Wraps librealsense2 syncer class
classdef syncer < handle
    properties (SetAccess = private, Hidden = true)
        objectHandle;
    end
    methods
        % Constructor
        function this = syncer(queue_size)
            if nargin == 0
                this.objectHandle = realsense.librealsense_mex('rs2::syncer', 'new');
            else
                validateattributes(queue_size, {'numeric'}, {'scalar', 'positive', 'real', 'integer'});
                this.objectHandle = realsense.librealsense_mex('rs2::syncer', 'new', uint64(queue_size));
            end
        end
        
        % Destructor
        function delete(this)
            if (this.objectHandle ~= 0)
                realsense.librealsense_mex('rs2::syncer', 'delete', this.objectHandle);
            end
        end
        
        % Functions
        function frames = wait_for_frames(this, timeout_ms)
            narginchk(1, 2);
            if nargin == 1
                out = realsense.librealsense_mex('rs2::syncer', 'wait_for_frames', this.objectHandle);
            else
                if isduration(timeout_ms)
                    timeout_ms = milliseconds(timeout_ms);
                end
                validateattributes(timeout_ms, {'numeric'}, {'scalar', 'nonnegative', 'real', 'integer'}, '', 'timeout_ms', 2);
                out = realsense.librealsense_mex('rs2::syncer', 'wait_for_frames', this.objectHandle, double(timeout_ms));
            end
            frames = realsense.frameset(out);
        end
        function [res, frames] = poll_for_frames(this)
            res, out = realsense.librealsense_mex('rs2::syncer', 'poll_for_frames', this.objectHandle);
            frames = realsense.frameset(out);
        end
    end
end