% Wraps librealsense2 video_stream_profile class
classdef video_stream_profile < realsense.stream_profile
    methods
        % Constructor
        function this = video_stream_profile(ownHandle, handle)
            this = this@realsense.stream_profile(ownHandle, handle);
        end

        % Destructor (uses base class destructor)

        % Functions
        function w = width(this)
            w = realsense.librealsense_mex('rs2::video_stream_profile', 'width', this.objectHandle);
        end
        function h = heigh(this)
            h = realsense.librealsense_mex('rs2::video_stream_profile', 'height', this.objectHandle);
        end
        function intrinsics = get_intrinsics(this)
            intrinsics = realsense.librealsense_mex('rs2::video_stream_profile', 'get_intrinsics', this.objectHandle);
        end
    end
end