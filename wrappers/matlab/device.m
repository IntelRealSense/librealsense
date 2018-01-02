% Wraps librealsense2 device class
classdef device < handle
    properties (SetAccess = private, Hidden = true)
        objectHandle;
        id;
    end
    methods (Access = private)
        function do_init(this)
            if (this.id >= 0)
                this.objectHandle = realsense.librealsense_mex('rs2::device', 'init', this.objectHandle, this.id);
                this.id = -1;
            end
        end
    end
    methods
        % Constructor
        function this = device(handle, index)
            this.objectHandle = handle;
            if (nargin == 1)  % constructed from device
                this.id = -1;
            else % lazily "constructed" from device_list
                this.id = index;
            end
        end
        % Destructor
        function delete(this)
            if (this.objectHandle ~= 0)
                if (this.id < 0) % still device list
                    realsense.librealsense_mex('rs2::device', 'delete_uninit', this.objectHandle);
                else
                    realsense.librealsense_mex('rs2::device', 'delete_init', this.objectHandle);
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
            this.do_init();
            out = realsense.librealsense_mex('rs2::device', 'first', this.objectHandle, type);
            switch (type)
                case 'sensor'
                    sensor = realsense.sensor(out);
                case 'roi_sensor'
                    sensor = realsense.roi_sensor(out);
                case 'depth_sensor'
                    sensor = realsense.depth_sensor(out);
            end
        end
%        function value = supports(this, info)
%            this.do_init();
%            value = realsense.librealsense_mex('rs2::device', 'supports', this.objectHandle, info);
%        end
%        function info = get_info(this, info) % TODO: name output var
%            this.do_init();
%            info = realsense.librealsense_mex('rs2::device', 'get_info', this.objectHandle, info);
%        end
        function hardware_reset(this)
            this.do_init();
            realsense.librealsense_mex('rs2::device', 'hardware_reset', this.objectHandle);
        end
        function value = is(this, type)
            this.do_init();
            out = realsense.librealsense_mex('rs2::device', 'is', this.objectHandle, type);
        end
%        function dev = as(this, type)
%            this.do_init();
%            out = realsense.librealsense_mex('rs2::device', 'as', this.objectHandle, type);
%            switch (type)
%                case ''
    end
end
