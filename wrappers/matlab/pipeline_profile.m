% Wraps librealsense2 pipeline_profile class
classdef pipeline_profile < handle
    properties (SetAccess = private, Hidden = true)
        objectHandle;
    end
    methods
        % Constructor
        function this = pipeline_profile(handle)
            narginchk(1, 1);
            validateattributes(handle, {'uint64'}, {'scalar'});
            this.objectHandle = handle;
        end
        % Destructor
        function delete(this)
            if (this.objectHandle ~= 0)
                realsense.librealsense_mex('rs2::pipeline_profile', 'delete', this.objectHandle);
            end
        end

        % Functions
        function streams = get_streams(this)
            arr = realsense.librealsense_mex('rs2::pipeline_profile', 'get_streams', this.objectHandle);
            % TOOD: Might be cell array
            streams = arrayfun(@(x) realsense.stream_profile(x{:}{:}), arr, 'UniformOutput', false);
        end
        function stream = get_stream(this, stream_type, stream_index)
            narginchk(2, 3);
            validateattributes(stream_type, {'realsense.stream', 'numeric'}, {'scalar', 'nonnegative', 'real', 'integer', '<=', realsense.stream.count}, '', 'stream_type', 2);
            if nargin == 2
                out = realsense.librealsense_mex('rs2::pipeline_profile', 'get_stream', this.objectHandle, int64(stream_type));
            else
                validateattributes(stream_index, {'numeric'}, {'scalar', 'nonnegative', 'real', 'integer'}, '', 'stream_index', 3);
                out = realsense.librealsense_mex('rs2::pipeline_profile', 'get_stream', this.objectHandle, int64(stream_type), int64(stream_index));
            end
            stream = realsense.stream_profile(out{:});
        end
        function dev = get_device(this)
            out = realsense.librealsense_mex('rs2::pipeline_profile', 'get_device', this.objectHandle);
            dev = realsense.device(out);
        end
    end
end