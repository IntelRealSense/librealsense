% Wraps librealsense2 sensor class
classdef sensor < handle
    properties (SetAccess = protected, Hidden = true)
        objectHandle;
    end
    methods
        % Constructor
        function this = sensor(handle)
            this.objectHandle = handle;
        end
        % Destructor
        function delete(this)
            if (this.objectHandle ~= 0)
                realsense.librealsense_mex('rs2::sensor', 'delete', this.objectHandle);
            end
        end

        % Functions
        
    end
end
