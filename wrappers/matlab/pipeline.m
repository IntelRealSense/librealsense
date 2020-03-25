% Wraps librealsense2 pipeline class
classdef pipeline < handle
    properties (SetAccess = protected, Hidden = true)
        objectHandle;
    end
    methods
        % Constructor
        function this = pipeline(ctx)
            if nargin == 0
                this.objectHandle = realsense.librealsense_mex('rs2::pipeline', 'new');
            else
                validateattributes(ctx, {'realsense.context'}, {'scalar'});
                this.objectHandle = realsense.librealsense_mex('rs2::pipeline', 'new', ctx.objectHandle);
            end
        end
        % Destructor
        function delete(this)
            if (this.objectHandle ~= 0)
                realsense.librealsense_mex('rs2::pipeline', 'delete', this.objectHandle);
            end
        end

        % Functions
        function pipeline_profile = start(this, varargin)
            narginchk(1, 3);
            if nargin == 1
                out = realsense.librealsense_mex('rs2::pipeline', 'start', this.objectHandle);
            elseif nargin == 2
                validateattributes(varargin{1}, {'realsense.config', 'realsense.frame_queue'}, {'scalar'}, '', 'config_or_framequeue', 2);
                if isa(varargin{1}, 'realsense.config')
                    out = realsense.librealsense_mex('rs2::pipeline', 'start', this.objectHandle, varargin{1}.objectHandle);
                else
                    out = realsense.librealsense_mex('rs2::pipeline', 'start#fq', this.objectHandle, varargin{1}.objectHandle);
                end
            else
                validateattributes(varargin{1}, {'realsense.config'}, {'scalar'}, '', 'config', 2);
                validateattributes(varargin{2}, {'realsense.frame_queue'}, {'scalar'}, '', 'frame_queue', 2);
                out = realsense.librealsense_mex('rs2::pipeline', 'start#fq', this.objectHandle, varargin{1}.objectHandle, varargin{2}.objectHandle);
            end
            pipeline_profile = realsense.pipeline_profile(out);
        end
        function stop(this)
            realsense.librealsense_mex('rs2::pipeline', 'stop', this.objectHandle);
        end
        function frames = wait_for_frames(this, timeout_ms)
            narginchk(1, 2);
            if nargin == 1
                out = realsense.librealsense_mex('rs2::pipeline', 'wait_for_frames', this.objectHandle);
            else
                if isduration(timeout_ms)
                    timeout_ms = milliseconds(timeout_ms);
                end
                validateattributes(timeout_ms, {'numeric'}, {'scalar', 'nonnegative', 'real', 'integer'}, '', 'timeout_ms', 2);
                out = realsense.librealsense_mex('rs2::pipeline', 'wait_for_frames', this.objectHandle, double(timeout_ms));
            end
            frames = realsense.frameset(out);
        end
        function [res, frames] = poll_for_frames(this)
            res, out = realsense.librealsense_mex('rs2::pipeline', 'poll_for_frames', this.objectHandle);
            frames = realsense.frameset(out);
        end
        function profile = get_active_profile(this)
            out = realsense.librealsense_mex('rs2::pipeline', 'get_active_profile', this.objectHandle);
            realsense.pipeline_profile(out);
        end
    end
end