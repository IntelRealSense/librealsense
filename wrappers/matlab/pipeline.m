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
        function pipeline_profile = start(this, config)
            narginchk(1, 2);
            if nargin == 1
                out = realsense.librealsense_mex('rs2::pipeline', 'start', this.objectHandle);
            else
                validateattributes(config, {'realsense.config'}, {'scalar'}, '', 'config', 2);
                out = realsense.librealsense_mex('rs2::pipeline', 'start', this.objectHandle, config.objectHandle);
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
        % TODO: poll_for_frames
        function profile = get_active_profile(this)
            out = realsense.librealsense_mex('rs2::pipeline', 'get_active_profile', this.objectHandle);
            realsense.pipeline_profile(out);
        end
    end
end