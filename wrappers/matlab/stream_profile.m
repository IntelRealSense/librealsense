% Wraps librealsense2 stream_profile class
classdef stream_profile < handle
    properties (SetAccess = private, Hidden = true)
        objectOwnHandle;
        objectHandle;
    end
    methods
        % Constructor
        function this = stream_profile(ownHandle, handle)
            this.objectOwnHandle = ownHandle;
            this.objectHandle = handle;
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
        % TODO: stream_type, format
        function f = fps(this)
            f = realsense.librealsense_mex('re2::stream_profile', 'fps', this.objectHandle);
        end
        function id = unique_id(this)
            id = realsense.librealsense_mex('re2::stream_profile', 'unique_id', this.objectHandle);
        end
        % TODO: clone
        function name = stream_name(this)
            id = realsense.librealsense_mex('re2::stream_profile', 'stream_name', this.objectHandle);
        end
        function value = is_default(this)
            value = realsense.librealsense_mex('re2::stream_profile', 'is_default', this.objectHandle);
        end
        % TODO: get_extrinsics_to
    end
end