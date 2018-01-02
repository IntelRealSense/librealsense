% Wraps librealsense2 pipeline class
classdef pipeline < handle
    properties (SetAccess = protected, Hidden = true)
        objectHandle;
    end
    methods
        % Constructor
        function this = pipeline(ctx)
            if  (nargin == 0)
                ctx = realsense.context();
            end
            if (~isa(ctx, 'realsense.context'))
                error('ctx must be of type realsense.context')
            end
            this.objectHandle = realsense.librealsense_mex('rs2::pipeline', 'new', ctx.objectHandle);
        end
        % Destructor
        function delete(this)
            if (this.objectHandle ~= 0)
                realsense.librealsense_mex('rs2::pipeline', 'delete', this.objectHandle);
            end
        end

        % Functions
        function start(this, config)
            % TODO: add output parameter after binding pipeline_profile
            switch nargin
                case 1
                    realsense.librealsense_mex('rs2::pipeline', 'start:void', this.objectHandle);
                case 2
                    if (isa(config, 'config')) 
                        % TODO: implement this overload
%                        realsense.librealsense_mex('rs2::pipeline', 'start:config', this.objectHandle, config.objectHandle);
                    else
                        % TODO: Error out meaningfully?
                        return
                    end
            end
        end
        function stop(this)
            realsense.librealsense_mex('rs2::pipeline', 'stop', this.objectHandle);
        end
        function frameset = wait_for_frames(this, timeout_ms)
            if (nargin == 1)
                timeout_ms = 5000;
            end
            frameset = realsense.frameset(realsense.librealsense_mex('rs2::pipeline', 'wait_for_frames', this.objectHandle, uint64(timeout_ms)));
        end
        % TODO: poll_for_frames, get_active_profile
    end
end