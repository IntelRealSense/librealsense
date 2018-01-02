function example1()
    % Make Pipeline object
    pipe = realsense.pipeline();
    % Start recording on arbitrary camera with default settings
    pipe.start();

    % Get a frameset
    fs = pipe.wait_for_frames();
    % Select depth frame
    depth = fs.get_depth_frame();
    % get actual data
    data = depth.get_data();

    % data is given as byte vector. convert into a matrix of uint16s
    rawimg = vec2mat(typecast(data, 'uint16'), depth.get_width());

    % convert to greyscale image
    img = mat2gray(rawimg, [0 double(max(rawimg(:)))]);
    imshow(img);
    pipe.stop();
end