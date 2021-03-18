classdef depth_view_example < matlab.apps.AppBase

    % Properties that correspond to app components
    properties (Access = public)
        UIFigure                  matlab.ui.Figure
        DepthAX                   matlab.ui.control.UIAxes
        ColorAX                   matlab.ui.control.UIAxes
        StartButton               matlab.ui.control.Button
        StopButton                matlab.ui.control.Button
    end

    properties (Access = private) %, Hidden = true)
        MeTimer            % Timer object
        DrawFlag           % Boolean 
        Ctx                % Realsense.context
        Dev                % Realsense.device
        Cfg                % Realsense.config
        Pipe               % Realsense.pipeline
        Colorizer          % Realsense.colorizer
        Profile            % Realsense.profile
        Frameset           % Realsense.frameset
        hhDepth            % Image
        hhColor            % Image        
    end

    % Callbacks that handle component events
    methods (Access = private)

        function MeTimerFcn(app,~,~)

        if ( app.DrawFlag == 1 )
           
           % lock drawing process
           app.DrawFlag = 0;
           
           % Get frameset
           app.Frameset = app.Pipe.wait_for_frames();
 
           depth_frame = app.Frameset.get_depth_frame();
           depth_color = app.Colorizer.colorize(depth_frame);
           depth_data = depth_color.get_data();
           depth_img = permute(reshape(depth_data',[3,depth_color.get_width(),depth_color.get_height()]),[3 2 1]);
           [ki kj] = size (app.hhDepth);
           if ki*kj < 1
            app.hhDepth = imshow(depth_img,'Parent',app.DepthAX);
           else
            app.hhDepth.CData = depth_img;
           end

           color_frame = app.Frameset.get_color_frame();
           color_data = color_frame.get_data();
           color_img = permute(reshape(color_data',[3,color_frame.get_width(),color_frame.get_height()]),[3 2 1]);
           [ki kj] = size (app.hhColor);
           if ki*kj < 1
            app.hhColor = imshow(color_img,'Parent',app.ColorAX);
           else
            app.hhColor.CData = color_img;
           end
           
           % unlock drawing process
           app.DrawFlag = 1;
           pause(0.001);   
        end
        
       end

        % Executes after component creation
        function StartUpFunc(app)
               % Create Realsense items
               app.Ctx = realsense.context();
               devs = app.Ctx.query_devices();

               if ( numel(devs) < 1 )
                error ( 'Not found RealSense device' );
               end

               app.Dev = devs{1};
               sn = app.Dev.get_info(realsense.camera_info.serial_number);
               app.Cfg = realsense.config();
               app.Cfg.enable_device(sn);
               app.Pipe = realsense.pipeline();
               app.Colorizer = realsense.colorizer();
               app.Profile = app.Pipe.start();

               % Create timer object

               kFramePerSecond = 30.0;                                  % Number of frames per second
               Period = double(int64(1000.0 / kFramePerSecond))/1000.0+0.001; % Frame Rate
               
               app.MeTimer = timer(...
                 'ExecutionMode', 'fixedSpacing', ...  % 'fixedRate', ...     % Run timer repeatedly
                 'Period', Period, ...                 % Period (second)
                 'BusyMode', 'drop', ... %'queue',...  % Queue timer callbacks when busy
                 'TimerFcn', @app.MeTimerFcn);         % Specify callback function

               app.DrawFlag = 0;
               app.hhDepth = [];
               app.hhColor = [];
               
        end

        % Button pushed function: start timer
        function onStartButton(app, event)
            % If timer is not running, start it
            if strcmp(app.MeTimer.Running, 'off')
               app.DrawFlag = 1;
               start(app.MeTimer);
            end
        end

        % Button pushed function: stop timer
        function onStopButton(app, event)
            app.DrawFlag = 0;
            stop(app.MeTimer);
        end

        %Close request UIFigure function
        function UIFigureCloseRequest(app,event)
            app.DrawFlag = 0;
            stop(app.MeTimer);
            delete(app.MeTimer);
            app.Pipe.stop();
            delete(app.Profile);
            delete(app.Colorizer);
            delete(app.Pipe);
            delete(app.Cfg);
            delete(app.Dev);
            delete(app.Ctx);
            delete(app);
        end

    end

    % Component initialization
    methods (Access = private)

        % Create UIFigure and components
        function createComponents(app)

            % Create UIFigure and hide until all components are created
            app.UIFigure = uifigure('Visible', 'off');
            app.UIFigure.Position = [20 20 1345 590];
            app.UIFigure.Name = 'RealSenseViewStdDef Figure';
            app.UIFigure.CloseRequestFcn = createCallbackFcn(app,@UIFigureCloseRequest);
            setAutoResize(app,app.UIFigure,false);

            % Create DepthAX
            app.DepthAX = uiaxes(app.UIFigure);
            app.DepthAX.Position = [25 100 640 480];

            % Create ColorAX
            app.ColorAX = uiaxes(app.UIFigure);
            app.ColorAX.Position = [685 100 640 480];

            % Create StartButton
            app.StartButton = uibutton(app.UIFigure, 'push');
            app.StartButton.ButtonPushedFcn = createCallbackFcn(app, @onStartButton, true);
            app.StartButton.IconAlignment = 'center';
            app.StartButton.Position = [100 22 100 22];
            app.StartButton.Text = 'Start';

            % Create StopButton
            app.StopButton = uibutton(app.UIFigure, 'push');
            app.StopButton.ButtonPushedFcn = createCallbackFcn(app, @onStopButton, true);
            app.StopButton.IconAlignment = 'center';
            app.StopButton.Position = [220 22 100 22];
            app.StopButton.Text = 'Stop';

            % Show the figure after all components are created
            app.UIFigure.Visible = 'on';
        end
    end

    % App creation and deletion
    methods (Access = public)

        % Construct app
        function app = depth_view_example

            % Create UIFigure and components
            createComponents(app)

            % Register the app with App Designer
            registerApp(app, app.UIFigure)

            % Set Startup function - after component creation
            runStartupFcn(app,@StartUpFunc);
            
            if nargout == 0
                clear app
            end            
        end

        % Code that executes before app deletion
        function delete(app)

            % Delete UIFigure when app is deleted
            delete(app.UIFigure)
        end
    end
end
