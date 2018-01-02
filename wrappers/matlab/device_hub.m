% Wraps librealsense2 device_hub class
classdef device_hub < handle
    properties (SetAccess = private, Hidden = true)
        objectHandle;
    end
    methods
        % Constructor
        function this = device_hub(context)
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
            device = realsense.device(realsense.librealsense_mex('rs2::device_hub', 'wait_for_device', this.objectHandle));
        end
        function value = is_connected(this, dev)
            value = realsense.librealsense_mex('rs2::device_hub', 'is_connected', this.objectHandle, dev.objectHandle);
        end
    end
end