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
        function l = logical(this)
            l = realsense.librealsense_mex('rs2::frame', 'operator bool', this.objectHandle);
        end
        function sensor = get_sensor(this)
            out = realsense.librealsense_mex('rs2::frame', 'get_sensor', this.objectHandle);
            sensor = realsense.sensor(out);
        end
        function timestamp = get_timestamp(this)
            timestamp = realsense.librealsense_mex('rs2::frame', 'get_timestamp', this.objectHandle);
        end
        function domain = get_frame_timestamp_domain(this)
            ret = realsense.librealsense_mex('rs2::frame', 'get_frame_timestamp_domain', this.objectHandle);
            domain = realsense.timestamp_domain(ret);
        end
        function metadata = get_frame_metadata(this, frame_metadata)
            narginchk(2, 2);
            validateattributes(frame_metadata, {'realsense.frame_metadata_value', 'numeric'}, {'scalar', 'nonnegative', 'real', 'integer', '<=', int64(realsense.frame_metadata_value.count)}, '', 'frame_metadata', 2);
            metadata = realsense.librealsense_mex('rs2::frame', 'get_frame_metadata', this.objectHandle, int64(frame_metadata));
        end
        function value = supports_frame_metadata(this, frame_metadata)
            narginchk(2, 2);
            validateattributes(frame_metadata, {'realsense.frame_metadata_value', 'numeric'}, {'scalar', 'nonnegative', 'real', 'integer', '<=', int64(realsense.frame_metadata_value.count)}, '', 'frame_metadata', 2);
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
        function value = is(this, type)
            narginchk(2, 2);
            % C++ function validates contents of type
            validateattributes(type, {'char', 'string'}, {'scalartext'}, '', 'type', 2);
            out = realsense.librealsense_mex('rs2::frame', 'is', this.objectHandle, type);
            value = logical(out);
        end
        function frame = as(this, type)
            narginchk(2, 2);
            % C++ function validates contents of type
            validateattributes(type, {'char', 'string'}, {'scalartext'}, '', 'type', 2);
            out = realsense.librealsense_mex('rs2::frame', 'as', this.objectHandle, type);
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
    end
end