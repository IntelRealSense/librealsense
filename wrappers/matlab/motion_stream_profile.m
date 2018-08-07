% Wraps librealsense2 motion_stream_profile class
classdef motion_stream_profile < realsense.stream_profile
    methods
        % Constructor
        function this = motion_stream_profile(ownHandle, handle)
            this = this@realsense.stream_profile(ownHandle, handle);
        end

        % Destructor (uses base class destructor)

        % Functions
        function motion_intrinsics = get_motion_intrinsics(this)
            intrinsics = realsense.librealsense_mex('rs2::motion_stream_profile', 'get_motion_intrinsics', this.objectHandle);
        end
    end
end