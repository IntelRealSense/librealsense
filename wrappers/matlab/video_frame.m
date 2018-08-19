% Wraps librealsense2 video_frame class
classdef video_frame < realsense.frame
    methods
        % Constructor
        function this = video_frame(handle)
            this = this@realsense.frame(handle);
        end

        % Destructor (uses base class destructor)
        
        % Functions
        function width = get_width(this)
            width = realsense.librealsense_mex('rs2::video_frame', 'get_width', this.objectHandle);
        end
        function height = get_height(this)
            height = realsense.librealsense_mex('rs2::video_frame', 'get_height', this.objectHandle);
        end
        function stride = get_stride_in_bytes(this)
            stride = realsense.librealsense_mex('rs2::video_frame', 'get_stride_in_bytes', this.objectHandle);
        end
        function bipp = get_bits_per_pixel(this)
            bipp = realsense.librealsense_mex('rs2::video_frame', 'get_bits_per_pixel', this.objectHandle);
        end
        function bpp = get_bytes_per_pixel(this)
            bpp = realsense.librealsense_mex('rs2::video_frame', 'get_bytes_per_pixel', this.objectHandle);
        end
    end
end