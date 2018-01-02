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
            device_array = arrayfun(@realsense.device, arr(:,1), arr(:,2), 'UniformOutput', false);
        end
        function sensor_array = query_all_sensors(this)
            arr = realsense.librealsense_mex('rs2::context', 'query_all_sensors', this.objectHandle);
            sensor_array = arrayfun(@realsense.sensor, arr, 'UniformOutput', false);
        end
        function device = get_sensor_parent(this, sensor)
            if (isa(sensor, 'sensor')) 
                device = realsense.device(realsense.librealsense_mex('rs2::context', 'get_sensor_parent', this.objectHandle, sensor.objectHandle));
            else
                error('sensor must be a sensor');
            end
        end
        function playback = load_device(this, file)
            % TODO: finalize once playback is wrapped
            playback = realsense.playback(realsense.librealsense_mex('rs2::context', 'load_device', this.objectHandle, file));
        end
        function unload_device(this, file)
            realsense.librealsense_mex('rs2::context', 'unload_device', this.objectHandle, file);
        end
    end
end