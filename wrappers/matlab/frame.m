% Wraps librealsense2 frame class
classdef frame < handle
    properties (SetAccess = protected, Hidden = true)
        objectHandle;
    end
    methods
        % Constructor
        function this = frame(handle)
            validateattributes(handle, {'uint64'}, {'scalar'});
            this.objectHandle = handle;
        end
        % Destructor
        function delete(this)
            if this.objectHandle ~= 0
                realsense.librealsense_mex('rs2::frame', 'delete', this.objectHandle);
            end
        end
        
        % Functions
        % TODO: keep
        function timestamp = get_timestamp(this)
            timestamp = realsense.librealsense_mex('rs2::frame', 'get_timestamp', this.objectHandle);
        end
        function domain = get_frame_timestamp_domain(this)
            ret = realsense.librealsense_mex('rs2::frame', 'get_frame_timestamp_domain', this.objectHandle);
            domain = realsense.timestamp_domain(ret);
        end
        function metadata = get_frame_metadata(this, frame_metadata)
            narginchk(2, 2);
            validateattributes(frame_metadata, {'realsense.frame_metadata_value', 'numeric'}, {'scalar', 'nonnegative', 'real', 'integer', '<=', realsense.frame_metadata_value.count}, '', 'frame_metadata', 2);
            metadata = realsense.librealsense_mex('rs2::frame', 'get_frame_metadata', this.objectHandle, int64(frame_metadata));
        end
        function value = supports_frame_metadata(this, frame_metadata)
            narginchk(2, 2);
            validateattributes(frame_metadata, {'realsense.frame_metadata_value', 'numeric'}, {'scalar', 'nonnegative', 'real', 'integer', '<=', realsense.frame_metadata_value.count}, '', 'frame_metadata', 2);
            value = realsense.librealsense_mex('rs2::frame', 'supports_frame_metadata', this.objectHandle, int64(frame_metadata));
        end
        function frame_number = get_frame_number(this)
            frame_number = realsense.librealsense_mex('rs2::frame', 'get_frame_number', this.objectHandle);
        end
        function data = get_data(this)
            data = realsense.librealsense_mex('rs2::frame', 'get_data', this.objectHandle);
        end
        function profile = get_profile(this)
            ret = realsense.librealsense_mex('rs2::frame', 'get_profile', this.objectHandle);
            profile = realsense.stream_profile(ret{:});
        end
        % TODO: is [frame, video_frame, points, depth_frame, disparity_frame, motion_frame, pose_frame, frameset]
        % TODO: as [frame, video_frame, points, depth_frame, disparity_frame, motion_frame, pose_frame, frameset]
    end
end