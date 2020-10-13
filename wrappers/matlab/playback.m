% Wraps librealsense2 playback class
classdef playback < realsense.device
    methods
        % Constructor
        function this = playback(handle, index)
           this = this@realsense.device(handle, index);
        end
        
        % Destructor (uses base class destructor)

        % Functions
        function pause(this)
            this.do_init();
            realsense.librealsense_mex('rs2::playback', 'pause', this.objectHandle);
        end
        function resume(this)
            this.do_init();
            realsense.librealsense_mex('rs2::playback', 'resume', this.objectHandle);
        end
        function fname = file_name(this)
            this.do_init();
            fname = realsense.librealsense_mex('rs2::playback', 'file_name', this.objectHandle);
        end
        function pos = get_position(this)
            this.do_init();
            pos = realsense.librealsense_mex('rs2::playback', 'get_position', this.objectHandle);
        end
        function dur = get_duration(this)
            this.do_init();
            out = realsense.librealsense_mex('rs2::playback', 'get_duration', this.objectHandle);
            dur = milliseconds(out);
        end
        function seek(this, time)
            narginchk(2, 2);
            validateattributes(time, {'duration'}, {'scalar', 'nonnegative'}, '', 'time', 2);
            this.do_init();
            realsense.librealsense_mex('rs2::playback', 'seek', this.objectHandle, milliseconds(time));
        end
        function value = is_real_time(this)
            this.do_init();
            value = realsense.librealsense_mex('rs2::playback', 'is_real_time', this.objectHandle);
        end
        function set_real_time(this, real_time)
            narginchk(2, 2);
            validateattributes(real_time, {'logical', 'numeric'}, {'scalar', 'real'}, '', 'real_time', 2);
            this.do_init();
            realsense.librealsense_mex('rs2::playback', 'set_real_time', this.objectHandle, logical(real_time));
        end
        function set_playback_speed(this, speed)
            narginchk(2, 2);
            validateattributes(speed, {'numeric'}, {'scalar', 'real'}, '', 'speed', 2);
            this.do_init();
            realsense.librealsense_mex('rs2::playback', 'set_playback_speed', this.objectHandle, double(speed));
        end
        function status = current_status(this)
            this.do_init();
            out = realsense.librealsense_mex('rs2::playback', 'current_status', this.objectHandle);
            status = realsense.playback_status(out);
        end
        function stop(this)
            this.do_init();
            realsense.librealsense_mex('rs2::playback', 'stop', this.objectHandle);
        end
    end
end