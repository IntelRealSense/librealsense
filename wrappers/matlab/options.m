% Wraps librealsense2 options class
classdef options < handle
    properties (SetAccess = protected, Hidden = true)
        objectHandle;
    end
    methods
        % Constructor
        function this = options(handle)
            narginchk(1, 1);
            validateattributes(handle, {'uint64'}, {'scalar'});
            this.objectHandle = handle;
        end
        % Destructor
        function delete(this)
            if this.objectHandle ~= 0
                realsense.librealsense_mex('rs2::options', 'delete', this.objectHandle);
            end
        end
        
        % Functions
        function value = supports_option(this, option)
            narginchk(2, 2);
            validateattributes(option, {'realsense.option', 'numeric'}, {'scalar', 'nonnegative', 'real', 'integer'}, '', 'option', 2);
            value = realsense.librealsense_mex('rs2::options', 'supports#rs2_option', this.objectHandle, int64(option));
        end
        function option_description = get_option_description(this, option)
            narginchk(2, 2);
            validateattributes(option, {'realsense.option', 'numeric'}, {'scalar', 'nonnegative', 'real', 'integer'}, '', 'option', 2);
            option_description = realsense.librealsense_mex('rs2::options', 'get_option_description', this.objectHandle, int64(option));
        end
        function option_value_description = get_option_value_description(this, option, val)
            narginchk(3, 3);
            validateattributes(option, {'realsense.option', 'numeric'}, {'scalar', 'nonnegative', 'real', 'integer'}, '', 'option', 2);
            validateattributes(val, {'numeric'}, {'scalar', 'real'}, '', 'val', 3);
            option_value_description = realsense.librealsense_mex('rs2::options', 'get_option_value_description', this.objectHandle, int64(option), double(val));
        end
        function value = get_option(this, option)
            narginchk(2, 2);
            validateattributes(option, {'realsense.option', 'numeric'}, {'scalar', 'nonnegative', 'real', 'integer'}, '', 'option', 2);
            value = realsense.librealsense_mex('rs2::options', 'get_option', this.objectHandle, int64(option));
        end
        function option_range = get_option_range(this, option)
            narginchk(2, 2);
            validateattributes(option, {'realsense.option', 'numeric'}, {'scalar', 'nonnegative', 'real', 'integer'}, '', 'option', 2);
            option_range = realsense.librealsense_mex('rs2::options', 'get_option_range', this.objectHandle, int64(option));
        end
        function set_option(this, option, val)
            narginchk(3, 3);
            validateattributes(option, {'realsense.option', 'numeric'}, {'scalar', 'nonnegative', 'real', 'integer'}, '', 'option', 2);
            validateattributes(val, {'numeric'}, {'scalar', 'real'}, '', 'val', 3);
            realsense.librealsense_mex('rs2::options', 'set_option', this.objectHandle, int64(option), double(val));
        end
        function value = is_option_read_only(this, option)
            narginchk(2, 2);
            validateattributes(option, {'realsense.option', 'numeric'}, {'scalar', 'nonnegative', 'real', 'integer'}, '', 'option', 2);
            value = realsense.librealsense_mex('rs2::options', 'is_option_read_only', this.objectHandle, int64(option));
        end
    end
end