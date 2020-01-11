% Wraps librealsense2 frameset class
classdef frameset < realsense.frame
    methods
        % Constructor
        function this = frameset(handle)
            this = this@realsense.frame(handle);
        end

        % Destructor (uses base class destructor)

        % Functions
        function frame = first_or_default(this, s)
            narginchk(2, 2);
            validateattributes(s, {'realsense.stream', 'numeric'}, {'scalar', 'nonnegative', 'real', 'integer', '<=', realsense.stream.count}, '', 's', 2);
            ret = realsense.librealsense_mex('rs2::frameset', 'first_or_default', this.objectHandle, int64(s));
            frame = realsense.frame(ret);
        end
        function frame = first(this, s)
            narginchk(2, 2);
            validateattributes(s, {'realsense.stream', 'numeric'}, {'scalar', 'nonnegative', 'real', 'integer', '<=', realsense.stream.count}, '', 's', 2);
            ret = realsense.librealsense_mex('rs2::frameset', 'first', this.objectHandle, int64(s));
            frame = realsense.frame(ret);
        end
        function depth_frame = get_depth_frame(this)
            ret = realsense.librealsense_mex('rs2::frameset', 'get_depth_frame', this.objectHandle);
            depth_frame = realsense.depth_frame(ret);
        end
        function color_frame = get_color_frame(this)
            ret = realsense.librealsense_mex('rs2::frameset', 'get_color_frame', this.objectHandle);
            color_frame = realsense.video_frame(ret);
        end
        function infrared_frame = get_infrared_frame(this, index)
            if (nargin == 1)
                ret = realsense.librealsense_mex('rs2::frameset', 'get_infrared_frame', this.objectHandle);
            else
                validateattributes(index, {'numeric'}, {'scalar', 'real', 'integer'}, '', 'index', 2);
                ret = realsense.librealsense_mex('rs2::frameset', 'get_infrared_frame', this.objectHandle, int64(index));
            end
            infrared_frame = realsense.video_frame(ret);
        end
        function fisheye_frame = get_fisheye_frame(this, index)
            if (nargin == 1)
                ret = realsense.librealsense_mex('rs2::frameset', 'get_fisheye_frame', this.objectHandle);
            else
                validateattributes(index, {'numeric'}, {'scalar', 'real', 'integer'}, '', 'index', 2);
                ret = realsense.librealsense_mex('rs2::frameset', 'get_fisheye_frame', this.objectHandle, int64(index));
            end
            fisheye_frame = realsense.video_frame(ret);
        end
        function pose_frame = get_pose_frame(this, index)
            if (nargin == 1)
                ret = realsense.librealsense_mex('rs2::frameset', 'get_pose_frame', this.objectHandle);
            else
                validateattributes(index, {'numeric'}, {'scalar', 'real', 'integer'}, '', 'index', 2);
                ret = realsense.librealsense_mex('rs2::frameset', 'get_pose_frame', this.objectHandle, int64(index));
            end
            pose_frame = realsense.video_frame(ret);
        end
        function frameset_size = frame_count(this)
            frameset_size = realsense.librealsense_mex('rs2::frameset', 'size', this.objectHandle);
        end
        % TODO: iterator protocol?
    end
end