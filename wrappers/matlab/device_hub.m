% Wraps librealsense2 device_hub class
classdef device_hub < handle
    properties (SetAccess = private, Hidden = true)
        objectHandle;
    end
    methods
        % Constructor
        function this = device_hub(context)
            narginchk(1, 1);
            validateattributes(context, {'realsense.context'}, {'scalar'});
            this.objectHandle = realsense.librealsense_mex('rs2::device_hub', 'new', context.objectHandle);
        end
        % Destructor
        function delete(this)
            if (this.objectHandle ~= 0)
                realsense.librealsense_mex('rs2::device_hub', 'delete', this.objectHandle);
            end
        end

        % Functions
        function device = wait_for_device(this)
            out = realsense.librealsense_mex('rs2::device_hub', 'wait_for_device', this.objectHandle)
            device = realsense.device(out(1), out(2));
        end
        function value = is_connected(this, dev)
            narginchk(2, 2);
            validateattributes(dev, {'realsense.device'}, {'scalar'}, '', 'dev', 2);
            value = realsense.librealsense_mex('rs2::device_hub', 'is_connected', this.objectHandle, dev.objectHandle);
        end
    end
end