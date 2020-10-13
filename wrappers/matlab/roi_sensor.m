% Wraps librealsense2 roi_sensor class
classdef roi_sensor < realsense.sensor
    methods
        % Constructor
        function this = roi_sensor(handle)
            this = this@realsense.sensor(handle);
        end
        
        % Destructor (uses base class destructor)

        % Functions
        function set_region_of_interest(this, roi)
            narginchk(2, 2)
            validateattributes(roi, {'struct'}, {'scalar'}, '', 'roi', 2);
            if ~isfield(roi, 'min_x')
                error('Expected input number 2, roi, to have a min_x field');;
            end
            validateattributes(roi.min_x, {'numeric'}, {'scalar', 'real', 'integer'}, '', 'roi.min_x', 2);
            if ~isfield(roi, 'min_y')
                error('Expected input number 2, roi, to have a min_y field');;
            end
            validateattributes(roi.min_y, {'numeric'}, {'scalar', 'real', 'integer'}, '', 'roi.min_y', 2);
            if ~isfield(roi, 'max_x')
                error('Expected input number 2, roi, to have a max_x field');;
            end
            validateattributes(roi.max_x, {'numeric'}, {'scalar', 'real', 'integer'}, '', 'roi.max_x', 2);
            if ~isfield(roi, 'max_y')
                error('Expected input number 2, roi, to have a min_x field');;
            end
            validateattributes(roi.max_y, {'numeric'}, {'scalar', 'real', 'integer'}, '', 'roi.max_y', 2);
            input = struct('min_x', int64(roi.min_x), 'min_y', int64(roi.min_y), 'max_x', int64(max_x), 'max_y', int64(max_y));
            realsense.librealsense_mex('rs2::roi_sensor', 'set_region_of_interest', this.objectHandle, input);
        end
        function roi = get_region_of_interest(this)
            roi = realsense.librealsense_mex('rs2::roi_sensor', 'get_region_of_interest', this.objectHandle);
        end
    end
end