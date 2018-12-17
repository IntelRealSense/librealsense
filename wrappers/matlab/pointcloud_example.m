function pointcloud_example()
    % Make Pipeline object to manage streaming
    pipe = realsense.pipeline();
    
    pointcloud = realsense.pointcloud();

    % Start streaming on an arbitrary camera with default settings
    profile = pipe.start();

    figure('visible','on');  hold on;
    figure('units','normalized','outerposition',[0 0 1 1])

    % Main loop
    for i = 1:10000

        % Obtain frames from a streaming device
        fs = pipe.wait_for_frames();
        
        % Select depth frame
        depth = fs.get_depth_frame();
        %color = fs.get_color_frame();

        % Produce pointcloud
        if (depth.logical())% && color.logical())

            %pointcloud.map_to(color);
            points = pointcloud.calculate(depth);
            
            % Adjust frame CS to matlab CS
            vertices = points.get_vertices();
            X = vertices(:,1,1);
            Y = vertices(:,2,1);
            Z = vertices(:,3,1);

            plot3(X,Z,-Y,'.');
            grid on
            hold off;
            view([45 30]);

            xlim([-0.5 0.5])
            ylim([0.3 1])
            zlim([-0.5 0.5])

            xlabel('X');
            ylabel('Z');
            zlabel('Y');
            
            pause(0.01);
        end
        % pcshow(vertices); Toolbox required
    end

     % Stop streaming
    pipe.stop();
    
end