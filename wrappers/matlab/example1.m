function example1()
    % Make Pipeline object
    pipe = realsense.pipeline();

    % Start streaming on an arbitrary camera with default settings
    profile = pipe.start();

    % Get streaming device's name
    dev = profile.get_device();
    name = dev.get_info(realsense.camera_info.name);

    % Get a frameset
    fs = pipe.wait_for_frames();
    
    % Stop streaming
    pipe.stop();

    % Select depth frame
    depth = fs.get_depth_frame();
    % get actual data
    data = depth.get_data();

    % data is returned by the wrapper as an unsigned byte vector.
    % Here we convert it into a matrix of uint16s
    rawimg = vec2mat(data, depth.get_width());

    % convert to greyscale image and display
    img = mat2gray(rawimg, [0 double(max(rawimg(:)))]);
    imshow(img);
    title(sprintf("Depth frame from %s", name));
end