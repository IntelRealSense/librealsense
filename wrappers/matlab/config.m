% Wraps librealsense2 config class
classdef config < handle
    properties (SetAccess = private, Hidden = true)
        objectHandle;
    end
    methods
        % Constructor
        function this = config()
            this.objectHandle = realsense.librealsense_mex('rs2::config', 'new');
        end
        % Destructor
        function delete(this)
            if (this.objectHandle ~= 0)
                realsense.librealsense_mex('rs2::config', 'delete', this.objectHandle);
            end
        end

        % Functions
        function enable_stream(this, varargin)
            narginchk(2, 7);
            validateattributes(varargin{1}, {'realsense.stream', 'numeric'}, {'scalar', 'nonnegative', 'real', 'integer', '<=', int64(realsense.stream.count)}, '', 'stream_type', 2);
            which = 'none';
            switch nargin
                case 2
                    which = 'enable_stream#stream';
                case 3
                    if isa(varargin{2}, 'realsense.format')
                        validateattributes(varargin{2}, {'realsense.format', 'numeric'}, {'scalar', 'nonnegative', 'real', 'integer', '<=', int64(realsense.format.count)}, '', 'format', 3);
                        which = 'enable_stream#format';
                    else
                        validateattributes(varargin{2}, {'numeric'}, {'scalar', 'real', 'integer'}, '', 'stream_index', 3);
                        which = 'enable_stream#stream';
                    end
                case 4
                    if isa(varargin{2}, 'realsense.format')
                        validateattributes(varargin{2}, {'realsense.format', 'numeric'}, {'scalar', 'nonnegative', 'real', 'integer', '<=', int64(realsense.format.count)}, '', 'format', 3);
                        validateattributes(varargin{3}, {'numeric'}, {'scalar', 'nonnegative', 'real', 'integer'}, '', 'framerate', 4);
                        which = 'enable_stream#format';
                    elseif isa(varargin{3}, 'realsense.format')
                        validateattributes(varargin{2}, {'numeric'}, {'scalar', 'real', 'integer'}, '', 'stream_index', 3);
                        validateattributes(varargin{3}, {'realsense.format', 'numeric'}, {'scalar', 'nonnegative', 'real', 'integer', '<=', int64(realsense.format.count)}, '', 'format', 4);
                        which = 'enable_stream#extended';
                    else
                        validateattributes(varargin{2}, {'numeric'}, {'scalar', 'nonnegative', 'real', 'integer'}, '', 'width', 3);
                        validateattributes(varargin{3}, {'numeric'}, {'scalar', 'nonnegative', 'real', 'integer'}, '', 'height', 4);
                        which = 'enable_stream#size';
                    end
                case 5
                    if isa(varargin{3}, 'realsense.format')
                        validateattributes(varargin{2}, {'numeric'}, {'scalar', 'real', 'integer'}, '', 'stream_index', 3);
                        validateattributes(varargin{3}, {'realsense.format', 'numeric'}, {'scalar', 'nonnegative', 'real', 'integer', '<=', int64(realsense.format.count)}, '', 'format', 4);
                        validateattributes(varargin{4}, {'numeric'}, {'scalar', 'nonnegative', 'real', 'integer'}, '', 'framerate', 5);
                        which = 'enable_stream#extended';
                    elseif isa(varargin{4}, 'realsense.format')
                        validateattributes(varargin{2}, {'numeric'}, {'scalar', 'nonnegative', 'real', 'integer'}, '', 'width', 3);
                        validateattributes(varargin{3}, {'numeric'}, {'scalar', 'nonnegative', 'real', 'integer'}, '', 'height', 4);
                        validateattributes(varargin{4}, {'realsense.format', 'numeric'}, {'scalar', 'nonnegative', 'real', 'integer', '<=', int64(realsense.format.count)}, '', 'format', 5);
                        which = 'enable_stream#size';
                    else
                        validateattributes(varargin{2}, {'numeric'}, {'scalar', 'real', 'integer'}, '', 'stream_index', 3);
                        validateattributes(varargin{3}, {'numeric'}, {'scalar', 'nonnegative', 'real', 'integer'}, '', 'width', 4);
                        validateattributes(varargin{4}, {'numeric'}, {'scalar', 'nonnegative', 'real', 'integer'}, '', 'height', 5);
                        which = 'enable_stream#full';
                    end
                case 6
                    if isa(varargin{4}, 'realsense.format')
                        validateattributes(varargin{2}, {'numeric'}, {'scalar', 'nonnegative', 'real', 'integer'}, '', 'width', 3);
                        validateattributes(varargin{3}, {'numeric'}, {'scalar', 'nonnegative', 'real', 'integer'}, '', 'height', 4);
                        validateattributes(varargin{4}, {'realsense.format', 'numeric'}, {'scalar', 'nonnegative', 'real', 'integer', '<=', int64(realsense.format.count)}, '', 'format', 5);
                        validateattributes(varargin{5}, {'numeric'}, {'scalar', 'nonnegative', 'real', 'integer'}, '', 'framerate', 6);
                        which = 'enable_stream#size';
                    else
                        validateattributes(varargin{2}, {'numeric'}, {'scalar', 'real', 'integer'}, '', 'stream_index', 3);
                        validateattributes(varargin{3}, {'numeric'}, {'scalar', 'nonnegative', 'real', 'integer'}, '', 'width', 4);
                        validateattributes(varargin{4}, {'numeric'}, {'scalar', 'nonnegative', 'real', 'integer'}, '', 'height', 5);
                        validateattributes(varargin{5}, {'realsense.format', 'numeric'}, {'scalar', 'nonnegative', 'real', 'integer', '<=', int64(realsense.format.count)}, '', 'format', 6);
                        which = 'enable_stream#full';
                    end
                case 7
                    validateattributes(varargin{2}, {'numeric'}, {'scalar', 'real', 'integer'}, '', 'stream_index', 3);
                    validateattributes(varargin{3}, {'numeric'}, {'scalar', 'nonnegative', 'real', 'integer'}, '', 'width', 4);
                    validateattributes(varargin{4}, {'numeric'}, {'scalar', 'nonnegative', 'real', 'integer'}, '', 'height', 5);
                    validateattributes(varargin{5}, {'realsense.format', 'numeric'}, {'scalar', 'nonnegative', 'real', 'integer', '<=', int64(realsense.format.count)}, '', 'format', 6);
                    validateattributes(varargin{6}, {'numeric'}, {'scalar', 'nonnegative', 'real', 'integer'}, '', 'framerate', 7);
                    which = 'enable_stream#full';
            end
            args = {size(varargin)};
            for i = 1:length(varargin)
                args{i} = int64(varargin{i});
            end
            realsense.librealsense_mex('rs2::config', which, this.objectHandle, args{:});
        end
        function enable_all_streams(this)
            realsense.librealsense_mex('rs2::config', 'enable_all_streams', this.objectHandle);
        end
        function enable_device(this, serial)
            narginchk(2, 2);
            validateattributes(serial, {'char', 'string'}, {'scalartext'}, '', 'serial', 2);
            realsense.librealsense_mex('rs2::config', 'enable_device', this.objectHandle, serial);
        end
        function enable_device_from_file(this, file_name, repeat_playback)
            narginchk(2, 3);
            validateattributes(file_name, {'char', 'string'}, {'scalartext'}, '', 'file_name', 2);
            if nargin==2
                realsense.librealsense_mex('rs2::config', 'enable_device_from_file', this.objectHandle, file_name);
            else
                validateattributes(repeat_playback, {'logical', 'numeric'}, {'scalar', 'real'}, '', 'repeat_playback', 3);
                realsense.librealsense_mex('rs2::config', 'enable_device_from_file', this.objectHandle, file_name, logical(repeat_playback));
            end
        end
        function enable_record_to_file(this, file_name)
            narginchk(2, 2);
            validateattributes(file_name, {'char', 'string'}, {'scalartext'}, '', 'file_name', 2);
            realsense.librealsense_mex('rs2::config', 'enable_record_to_file', this.objectHandle, file_name);
        end
        function disable_stream(this, stream, index)
            narginchk(2, 3);
            validateattributes(stream, {'realsense.stream', 'numeric'}, {'scalar', 'nonnegative', 'real', 'integer', '<=', int64(realsense.stream.count)}, '', 'stream', 2);
            if nargin == 2
                out = realsense.librealsense_mex('rs2::config', 'disable_stream', this.objectHandle, int64(stream));
            else
                validateattributes(index, {'numeric'}, {'scalar', 'real', 'integer'}, '', 'index', 3);
                out = realsense.librealsense_mex('rs2::config', 'disable_stream', this.objectHandle, int64(stream), int64(index));
            end
            stream = realsense.stream_profile(out{:});
        end
        function disable_all_streams(this)
            realsense.librealsense_mex('rs2::config', 'disable_all_streams', this.objectHandle);
        end
    end
end
