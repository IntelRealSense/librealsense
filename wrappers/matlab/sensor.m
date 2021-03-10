% Wraps librealsense2 sensor class
classdef sensor < realsense.options
    methods
        % Constructor
        function this = sensor(handle)
            this = this@realsense.options(handle);
        end
        
        % Destructor (uses base class destructor)

        % Functions
        function open(this, profiles)
            narginchk(2, 2)
            validateattributes(profiles, {'realsense.stream_profile'}, {'nonempty', 'vector'}, '', 'profiles', 2);
            if isscalar(profiles)
                realsense.librealsense_mex('rs2::sensor', 'open#stream_profile', this.objectHandle, profiles.objectHandle);
            else
                realsense.librealsense_mex('rs2::sensor', 'open#vec_stream_profile', this.objectHandle, profiles);
            end
        end
        function value = supports_camera_info(this, info)
            narginchk(2, 2)
            validateattributes(info, {'realsense.camera_info', 'numeric'},{'scalar', 'nonnegative', 'real', 'integer', '<=', int64(realsense.camera_info.count)}, '', 'info', 2);
            value = realsense.librealsense_mex('rs2::sensor', 'supports#rs2_camera_info', this.objectHandle, int64(info));
        end
        function value = get_info(this, info)
            narginchk(2, 2)
            validateattributes(info, {'realsense.camera_info', 'numeric'},{'scalar', 'nonnegative', 'real', 'integer', '<=', int64(realsense.camera_info.count)}, '', 'info', 2);
            value = realsense.librealsense_mex('rs2::sensor', 'get_info', this.objectHandle, int64(info));
        end
        function close(this)
            realsense.librealsense_mex('rs2::sensor', 'close', this.objectHandle);
        end
        % TODO: start [etc?]
        function start(this, queue)
            narginchk(2, 2);
            validateattributes(queue, {'realsense.frame_queue'}, {'scalar'}, '', 'queue', 2);
            realsense.librealsense_mex('rs2::sensor', 'start', this.objectHandle, queue.objectHandle);
        end
        function stop(this)
            realsense.librealsense_mex('rs2::sensor', 'stop', this.objectHandle);
        end
        function profiles = get_stream_profiles(this)
            arr = realsense.librealsense_mex('rs2::sensor', 'get_stream_profiles', this.objectHandle);
            % TODO: Might be cell array
            profiles = arrayfun(@(x) realsense.stream_profile(x{:}{:}), arr, 'UniformOutput', false);
        end
        function l = logical(this)
            l = realsense.librealsense_mex('rs2::sensor', 'operator bool', this.objectHandle);
        end
        function value = is(this, type)
            narginchk(2, 2);
            % C++ function validates contents of type
            validateattributes(type, {'char', 'string'}, {'scalartext'}, '', 'type', 2);
            out = realsense.librealsense_mex('rs2::sensor', 'is', this.objectHandle, type);
            value = logical(out);
        end
        function sensor = as(this, type)
            narginchk(2, 2);
            % C++ function validates contents of type
            validateattributes(type, {'char', 'string'}, {'scalartext'}, '', 'type', 2);
            out = realsense.librealsense_mex('rs2::sensor', 'as', this.objectHandle, type);
            switch type
                case 'sensor'
                    sensor = realsense.sensor(out);
                case 'roi_sensor'
                    sensor = realsense.roi_sensor(out);
                case 'depth_sensor'
                    sensor = realsense.depth_sensor(out);
                case 'depth_stereo_sensor'
                    sensor = realsense.depth_stereo_sensor(out);;
            end
        end

        % Operators
        % TODO: operator==
    end
end
