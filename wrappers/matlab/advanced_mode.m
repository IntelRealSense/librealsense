% Wraps librealsense2 advanced_mode class
classdef advanced_mode < realsense.device
    methods
        % Constructor
        function this = advanced_mode(handle, index)
           this = this@realsense.device(handle, index);
        end
        
        % Destructor (uses base class destructor)

        % Functions
        function enabled = is_enabled(this)
            this.do_init();
            enabled = realsense.librealsense_mex('rs400::advanced_mode', 'is_enabled', this.objectHandle);
        end
        function toggle_advanced_mode(this, enable)
            narginchk(2, 2);
            validateattributes(enable, {'logical', 'numeric'}, {'scalar', 'real'}, '', 'enable', 2);
            this.do_init();
            realsense.librealsense_mex('rs400::advanced_mode', 'toggle_advanced_mode', this.objectHandle, logical(enable));
        end
        function json_content = serialize_json(this)
            this.do_init();
            json_content = realsense.librealsense_mex('rs400::advanced_mode', 'serialize_json', this.objectHandle);
        end
        function load_json(this, json_content)
            narginchk(2, 2);
            validateattributes(json_content, {'string', 'char'}, {'scalartext', 'nonempty'}, '', 'json_content', 2);
            this.do_init();
            realsense.librealsense_mex('rs400::advanced_mode', 'load_json', this.objectHandle, json_content);
        end
    end
end