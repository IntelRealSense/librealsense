% Wraps librealsense2 stream_profile class
classdef stream_profile < handle
    properties (SetAccess = protected, Hidden = true)
        objectOwnHandle;
        objectHandle;
    end
    methods
        % Constructor
        function this = stream_profile(handle, ownHandle)
            narginchk(2, 2);
            validateattributes(handle, {'uint64'}, {'scalar'});
            validateattributes(ownHandle, {'uint64'}, {'scalar'});
            this.objectHandle = handle;
            this.objectOwnHandle = ownHandle;
        end
        % Destructor
        function delete(this)
            if (this.objectHandle ~= 0)
                realsense.librealsense_mex('rs2::stream_profile', 'delete', this.objectOwnHandle);
            end
        end

        % Functions
        function index = stream_index(this)
            index = realsense.librealsense_mex('rs2::stream_profile', 'stream_index', this.objectHandle);
        end
        function type = stream_type(this)
            type = realsense.stream(realsense.librealsense_mex('rs2::stream_profile', 'stream_type', this.objectHandle));
        end
        function fmt = format(this)
            fmt = realsense.format(realsense.librealsense_mex('rs2::stream_profile', 'format', this.objectHandle));
        end
        function f = fps(this)
            f = realsense.librealsense_mex('rs2::stream_profile', 'fps', this.objectHandle);
        end
        function id = unique_id(this)
            id = realsense.librealsense_mex('rs2::stream_profile', 'unique_id', this.objectHandle);
        end
        function profile = clone(this, type, index, fmt)
            narginchk(4, 4);
            validateattributes(type, {'realsense.stream', 'numeric'}, {'scalar', 'nonnegative', 'real', 'integer', '<=', int64(realsense.stream.count)}, '', 'type', 2);
            validateattributes(index, {'numeric'}, {'scalar', 'nonnegative', 'real', 'integer'}, '', 'index', 3);
            validateattributes(fmt, {'realsense.format', 'numeric'}, {'scalar', 'nonnegative', 'real', 'integer', '<=', int64(realsense.format.count)}, '', 'fmt', 4);
            out = realsense.librealsense_mex('rs2::stream_profile', 'clone', this.objectHandle, int64(type), int64(index), int64(fmt));
            profile = realsense.stream_profile(out{:});
        end
        function value = is(this, type)
            narginchk(2, 2);
            % C++ function validates contents of type
            validateattributes(type, {'char', 'string'}, {'scalartext'}, '', 'type', 2);
            out = realsense.librealsense_mex('rs2::stream_profile', 'is', this.objectHandle, type);
            value = logical(out);
        end
        function profile = as(this, type)
            narginchk(2, 2);
            % C++ function validates contents of type
            validateattributes(type, {'char', 'string'}, {'scalartext'}, '', 'type', 2);
            out = realsense.librealsense_mex('rs2::stream_profile', 'as', this.objectHandle, type);
            switch type
                case 'stream_profile'
                    profile = realsense.stream_profile(out{:});
                case 'video_stream_profile'
                    profile = realsense.video_stream_profile(out{:});
                case 'motion_stream_profile'
                    profile = realsense.motion_stream_profile(out{:});
            end
        end
        function name = stream_name(this)
            id = realsense.librealsense_mex('rs2::stream_profile', 'stream_name', this.objectHandle);
        end
        function value = is_default(this)
            value = realsense.librealsense_mex('rs2::stream_profile', 'is_default', this.objectHandle);
        end
        function l = logical(this)
            l = realsense.librealsense_mex('rs2::stream_profile', 'operator bool', this.objectHandle);
        end
        function extrinsics = get_extrinsics_to(this, to)
            narginchk(2, 2);
            validateattributes(to, {'realsense.stream_profile'}, {'scalar'}, '', 'to', 2);
            extrinsics = realsense.librealsense_mex('rs2::stream_profile', 'get_extrinsics_to', this.objectHandle, to.objectHandle);
        end
        function value = is_cloned(this)
            value = realsense.librealsense_mex('rs2::stream_profile', 'is_cloned', this.objectHandle)
        end

        % Operators
        % TODO: operator==
    end
end