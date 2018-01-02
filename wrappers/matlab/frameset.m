% Wraps librealsense2 video_frame class
classdef frameset < realsense.frame
    methods
        % Constructor
        function this = frameset(handle)
            this = this@realsense.frame(handle);
        end

        % Destructor (uses base class destructor)

        % Functions
        function frame = first_or_default(this, s)
            if (~isa(s, 'stream'))
                error('s must be a stream');
            end
            frame = realsense.frame(realsense.librealsense_mex('rs2::frameset', 'first_or_default', this.objectHandle, uint64_t(s)));
        end
        function frame = first(this, s)
            if (~isa(s, 'stream'))
                error('s must be a stream');
            end
            frame = realsense.frame(realsense.librealsense_mex('rs2::frameset', 'first', this.objectHandle, uint64_t(s)));
        end
        function depth_frame = get_depth_frame(this)
            depth_frame = realsense.depth_frame(realsense.librealsense_mex('rs2::frameset', 'get_depth_frame', this.objectHandle));
        end
        function video_frame = get_color_frame(this)
            video_frame = realsense.video_frame(realsense.librealsense_mex('rs2::frameset', 'get_color_frame', this.objectHandle));
        end
        function size = get_size(this)
            size = realsense.librealsense_mex('rs2::frameset', 'get_size', this.objectHandle);
        end
    end
end