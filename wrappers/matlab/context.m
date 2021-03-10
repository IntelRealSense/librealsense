% Wraps librealsense2 context class
classdef context < handle
    properties (SetAccess = private, Hidden = true)
        objectHandle;
    end
    methods
        % Constructor
        function this = context()
            this.objectHandle = realsense.librealsense_mex('rs2::context', 'new');
        end
        
        % Destructor
        function delete(this)
            if (this.objectHandle ~= 0)
                realsense.librealsense_mex('rs2::context', 'delete', this.objectHandle);
            end
        end
        
        % Functions
        function device_array = query_devices(this)
            arr = realsense.librealsense_mex('rs2::context', 'query_devices', this.objectHandle);
            % TODO: Might be cell array
            device_array = arrayfun(@(x) realsense.device(x{:}{:}), arr, 'UniformOutput', false);
        end
        function sensor_array = query_all_sensors(this)
            arr = realsense.librealsense_mex('rs2::context', 'query_all_sensors', this.objectHandle);
            sensor_array = arrayfun(@realsense.sensor, arr, 'UniformOutput', false);
        end
        function device = get_sensor_parent(this, sensor)
            narginchk(2, 2);
            validateattributes(sensor, {'realsense.sensor'}, {'scalar'}, '', 'sensor', 2);
            out = realsense.librealsense_mex('rs2::context', 'get_sensor_parent', this.objectHandle, sensor.objectHandle);
            device = realsense.device(out);
        end
        function playback = load_device(this, file)
            narginchk(2, 2);
            validateattributes(file, {'string', 'char'}, {'scalartext', 'nonempty'}, '', 'file', 2);
            out = realsense.librealsense_mex('rs2::context', 'load_device', this.objectHandle, file);
            playback = realsense.playback(out(1), out(2));
        end
        function unload_device(this, file)
            narginchk(2, 2);
            validateattributes(file, {'string', 'char'}, {'scalartext', 'nonempty'}, '', 'file', 2);
            realsense.librealsense_mex('rs2::context', 'unload_device', this.objectHandle, file);
        end
        function unload_tracking_module(this)
            realsense.librealsense_mex('rs2::context', 'unload_tracking_module', this.objectHandle);
        end
    end
end