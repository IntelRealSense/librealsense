% Wraps librealsense2 device class
classdef device < handle
    properties (SetAccess = private, Hidden = true)
        objectHandle;
        id;
    end
    methods (Access = protected)
        function do_init(this)
            if (this.id >= 0)
                this.objectHandle = realsense.librealsense_mex('rs2::device', 'init', this.objectHandle, uint64(this.id));
                this.id = -1;
            end
        end
    end
    methods
        % Constructor
        function this = device(handle, index)
            narginchk(1, 2);
            validateattributes(handle, {'uint64'}, {'scalar'});
            this.objectHandle = handle;
            if (nargin == 1)  % constructed from device
                this.id = -1;
            else % lazily "constructed" from device_list
                validateattributes(index, {'numeric'}, {'scalar', 'real', 'integer'});
                this.id = int64(index);
            end
        end
        % Destructor
        function delete(this)
            if (this.objectHandle ~= 0)
                if (this.id < 0) % still device list
                    realsense.librealsense_mex('rs2::device', 'delete#uninit', this.objectHandle);
                else
                    realsense.librealsense_mex('rs2::device', 'delete#init', this.objectHandle);
                end
            end
        end

        % Functions
        function sensor_array = query_sensors(this)
            this.do_init();
            arr = realsense.librealsense_mex('rs2::device', 'query_sensors', this.objectHandle);
            [sensor_array{1:nargout}] = arrayfun(@realsense.sensor, arr, 'UniformOutput', false);
        end
        function sensor = first(this, type)
            narginchk(2, 2);
            % C++ function validates contents of type
            validateattributes(type, {'char', 'string'}, {'scalartext'}, '', 'type', 2);
            this.do_init();
            out = realsense.librealsense_mex('rs2::device', 'first', this.objectHandle, type);
            sensor = realsense.sensor(out).as(type);
        end
        function value = supports(this, info)
            narginchk(2, 2);
            validateattributes(info, {'realsense.camera_info', 'numeric'}, {'scalar', 'nonnegative', 'real', 'integer', '<=', int64(realsense.camera_info.count)}, '', 'info', 2);
            this.do_init();
            value = realsense.librealsense_mex('rs2::device', 'supports', this.objectHandle, int64(info));
        end
        function info = get_info(this, info)
            narginchk(2, 2);
            validateattributes(info, {'realsense.camera_info', 'numeric'}, {'scalar', 'nonnegative', 'real', 'integer', '<=', int64(realsense.camera_info.count)}, '', 'info', 2);
            this.do_init();
            info = realsense.librealsense_mex('rs2::device', 'get_info', this.objectHandle, int64(info));
        end
        function hardware_reset(this)
            this.do_init();
            realsense.librealsense_mex('rs2::device', 'hardware_reset', this.objectHandle);
        end
        function l = logical(this)
            l = realsense.librealsense_mex('rs2::device', 'operator bool', this.objectHandle);
        end
        function value = is(this, type)
            narginchk(2, 2);
            % C++ function validates contents of type
            validateattributes(type, {'char', 'string'}, {'scalartext'}, '', 'type', 2);
            this.do_init();
            out = realsense.librealsense_mex('rs2::device', 'is', this.objectHandle, type);
            value = logical(out);
        end
        function dev = as(this, type)
            narginchk(2, 2);
            % C++ function validates contents of type
            validateattributes(type, {'char', 'string'}, {'scalartext'}, '', 'type', 2);
            this.do_init();
            out = realsense.librealsense_mex('rs2::device', 'as', this.objectHandle, type);
            switch type
                case 'device'
                    dev = realsense.device(out, -1);
                case 'debug_protocol'
                    error('debug_protocol is not supported in Matlab');
                case 'advanced_mode'
                    dev = realsense.advanced_mode(out, -1);
                case 'recorder'
                    dev = realsense.recorder(out, -1);
                case 'playback'
                    dev = realsense.playback(out, -1);
            end
        end
    end
end
