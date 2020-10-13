function rosbag_example(filename)
    % Make Config object to manage pipeline settings
    cfg = realsense.config();
    validateattributes(filename, {'char','string'}, {'scalartext', 'nonempty'}, '', 'filename', 1);
    % Tell pipeline to stream from the given rosbag file
    cfg.enable_device_from_file(filename)

    % Make Pipeline object to manage streaming
    pipe = realsense.pipeline();
    % Make Colorizer object to prettify depth output
    colorizer = realsense.colorizer();

    % Start streaming from the rosbag with default settings
    profile = pipe.start(cfg);

    % Get streaming device's name
    dev = profile.get_device();
    name = dev.get_info(realsense.camera_info.name);

    % Get frames. We discard the first couple to allow
    % the camera time to settle
    for i = 1:5
        fs = pipe.wait_for_frames();
    end
    
    % Stop streaming
    pipe.stop();

    % Select depth frame
    depth = fs.get_depth_frame();
    % Colorize depth frame
    color = colorizer.colorize(depth);

    % Get actual data and convert into a format imshow can use
    % (Color data arrives as [R, G, B, R, G, B, ...] vector)
    data = color.get_data();
    img = permute(reshape(data',[3,color.get_width(),color.get_height()]),[3 2 1]);
    
    % Display image
    imshow(img);
    title(sprintf("Colorized depth frame from %s", name));
end
