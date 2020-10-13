% Wraps librealsense2 recorder class
classdef recorder < realsense.device
    methods
        % Constructor
        function this = recorder(device, other)
            narginchk(2, 2);
            validateattributes(device, {'uint64', 'realsense.device'}, 'scalar');
            if isa(device, 'realsense.device')
                validateattributes(other, {'string', 'char'}, {'scalartext', 'nonempty'});
                out = realsense.librealsense_mex('rs2::recorder', 'new#string_device', other, device.objectHandle);
                this = this@realsense.device(out{:});
            else
                this = this@realsense.device(handle, other);
            end
        end
        
        % Destructor (uses base class destructor)

        % Functions
        function pause(this)
            this.do_init();
            realsense.librealsense_mex('rs2::recorder', 'pause', this.objectHandle);
        end
        function resume(this)
            this.do_init();
            realsense.librealsense_mex('rs2::recorder', 'resume', this.objectHandle);
        end
        function fname = filename(this)
            this.do_init();
            fname = realsense.librealsense_mex('rs2::recorder', 'filename', this.objectHandle);
        end
    end
end