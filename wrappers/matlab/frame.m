% Wraps librealsense2 frame class
classdef frame < handle
    properties (SetAccess = protected, Hidden = true)
        objectHandle;
    end
    methods
        % Constructor
        function this = frame(handle)
            if nargin == 0
                handle = 0;
            end
            this.objectHandle = handle;
        end
        % Destructor
        function delete(this)
            if this.objectHandle ~= 0
                realsense.librealsense_mex('rs2::frame', 'delete', this.objectHandle);
            end
        end
        
        % Functions
        function timestamp = get_timestamp(this)
            timestamp = realsense.librealsense_mex('rs2::frame', 'get_timestamp', this.objectHandle);
        end
        function domain = get_frame_timestamp_domain(this)
            domain = realsense.frame_timestamp_domain(realsense.librealsense_mex('rs2::frame', 'get_frame_timestamp_domain', this.objectHandle));
        end
        function metadata = get_frame_metadata(this, frame_metadata)
            if (~isa(frame_metadata, 'frame_metadata'))
                error('frame_metadata must be a frame_metadata');
            end
            metadata = realsense.librealsense_mex('rs2::frame', 'get_frame_metadata', this.objectHandle, uint64(frame_metadata));
        end
        function value = supports_frame_metadata(this, frame_metadata)
            if (~isa(frame_metadata, 'frame_metadata'))
                error('frame_metadata must be a frame_metadata');
            end
            value = realsense.librealsense_mex('rs2::frame', 'supports_frame_metadata', this.objectHandle, uint64(frame_metadata));
        end
        function frame_number = get_frame_number(this)
            timestamp = realsense.librealsense_mex('rs2::frame', 'get_frame_number', this.objectHandle);
        end
        function data = get_data(this)
            data = realsense.librealsense_mex('rs2::frame', 'get_data', this.objectHandle);
        end
        function profile = get_profile(this)
            profile = realsense.stream_profile(realsense.librealsense_mex('rs2::frame', 'get_profile', this.objectHandle));
        end
    end
end