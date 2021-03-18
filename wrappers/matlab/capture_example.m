classdef capture_example < matlab.apps.AppBase

    % Properties that correspond to app components
    properties (Access = public)
        UIFigure                  matlab.ui.Figure
        DepthAX                   matlab.ui.control.UIAxes
        ColorAX                   matlab.ui.control.UIAxes
        AccelAX                   matlab.ui.control.UIAxes
        AccelTextAX               matlab.ui.control.UIAxes
        GyroAX                    matlab.ui.control.UIAxes
        GyroTextAX                matlab.ui.control.UIAxes
        StartButton               matlab.ui.control.Button
        StopButton                matlab.ui.control.Button
        RsPipeLine                realsense.pipeline
        RsColorizer               realsense.colorizer
    end

    properties (Access = private, Hidden = true)
        tmr                % Timer object
        SupportDepth       % Support Depth Chanel
        SupportColor       % Support Color Chanel
        SupportAccel       % Support Accel Chanel
        SupportGyro        % Support Gyro  Chanel
        hD                 % Depth image handle
        hC                 % Color image handle
        hA                 % Accel handles
                           % hA{1} - Circle Accel.x0y 
                           % hA{2} - Circle Accel.x0z 
                           % hA{3} - Circle Accel.y0z
                           % hA{4} - Vect Accel.xyz
                           % hA{5} - String Accel.x
                           % hA{6} - String Accel.y
                           % hA{7} - String Accel.z
                           % hA{8} - String Accel.r
        hG                 % Gyro handles
                           % hG{1} - Circle Gyro.x0y 
                           % hG{2} - Circle Gyro.x0z 
                           % hG{3} - Circle Gyro.y0z
                           % hG{4} - Vect Gyro.xyz
                           % hG{5} - String Gyro.x
                           % hG{6} - String Gyro.y
                           % hG{7} - String Gyro.z
                           % hG{8} - String Gyro.r
        dV                 % Data vectors
                           % dV{1} - ort line  = - 1.15 .. 1.15 
                           % dV{2} - sin(t) , t = 0 .. 2 pi
                           % dV{3} - cos(t)
                           % dV{4} - zero = t ; zero(:) = 0;
        Period             % double ( sec. )
    end

    methods (Access = private)
    
        function set_motion_data(app,h,d)
            x = d(1); y = d(2); z = d(3); R = sqrt(x^2+y^2+z^2);
            nx = x/R; ny = y/R; nz = z/R;
            zz = app.dV{4}; zz(:) = nz; rr = sqrt(nx^2+ny^2);
            xx = rr*app.dV{3}; yy = rr*app.dV{2};
            h{1}.XData = xx; h{1}.YData = yy; h{1}.ZData = zz;
            xx = app.dV{4}; xx(:) = nx; rr = sqrt(ny^2+nz^2);
            yy = rr*app.dV{3}; zz = rr*app.dV{2};
            h{3}.XData = xx; h{3}.YData = yy; h{3}.ZData = zz;
            yy = app.dV{4}; yy(:) = ny; rr = sqrt(nx^2+nz^2);
            xx = rr*app.dV{3}; zz = rr*app.dV{2};
            h{2}.XData = xx; h{2}.YData = yy; h{2}.ZData = zz;
            h{4}.XData = [0 nx];
            h{4}.YData = [0 ny];
            h{4}.ZData = [0 nz];
            h{5}.String = sprintf('x = %f',x);
            h{6}.String = sprintf('y = %f',y);
            h{7}.String = sprintf('z = %f',z);
            h{8}.String = sprintf('r = %f',R);
        end
     
        function h = draw_motion_data(app,ag,at,d,ss)
            colorG = [0.1 0.7 0.2];
            x = d(1); y = d(2); z = d(3); R = sqrt(x^2+y^2+z^2);
            nx = x/R; ny = y/R; nz = z/R;
            plot3(ag,app.dV{3},app.dV{2},app.dV{4},'LineStyle','-','Color','b','linewidth',1);
            hold(ag,'on');
            plot3(ag,app.dV{4},app.dV{3},app.dV{2},'LineStyle','-','Color','r','linewidth',1);
            plot3(ag,app.dV{3},app.dV{4},app.dV{2},'LineStyle','-','Color',colorG,'linewidth',1);
            zz = app.dV{4}; zz(:) = nz; rr = sqrt(nx^2+ny^2);
            xx = rr * app.dV{3}; yy = rr * app.dV{2};
            h{1}=plot3(ag,xx,yy,zz,'LineStyle','--','Color','b','linewidth',1);
            yy = app.dV{4}; yy(:) = ny; rr = sqrt(nx^2+nz^2);
            xx = rr * app.dV{3}; zz = rr * app.dV{2};
            h{2}=plot3(ag,xx,yy,zz,'LineStyle','--','Color',colorG,'linewidth',1);
            xx = app.dV{4}; xx(:) = nx; rr = sqrt(ny^2+nz^2);
            yy = rr * app.dV{3}; zz = rr * app.dV{2};
            h{3}=plot3(ag,xx,yy,zz,'LineStyle','--','Color','r','linewidth',1);
            xx(:) = 0; yy(:) = 0; zz = app.dV{1};
            plot3(ag,xx,yy,zz,'LineStyle','-','Color','b','linewidth',1);
            xx(:) = 0; yy(:) = app.dV{1}; zz(:) = 0;
            plot3(ag,xx,yy,zz,'LineStyle','-','Color',colorG,'linewidth',1);
            xx(:) = app.dV{1}; yy(:) = 0; zz(:) = 0;
            plot3(ag,xx,yy,zz,'LineStyle','-','Color','r','linewidth',1);
            h{4}=line(ag,[0 nx],[0 ny],[0 nz]); h{4}.Color = 'k';
            axis(ag,'off'); hold(ag,'off');
            text(at,0.3,0.8,ss,'Color',[0,0,0],'FontSize',14); hold(at,'on');
            s=sprintf('x = %f',x);
            h{5}=text(at,0.2,0.7,s,'Color','r','FontSize',12);
            s=sprintf('y = %f',y);
            h{6}=text(at,0.2,0.6,s,'Color',colorG,'FontSize',12);
            s=sprintf('z = %f',z);
            h{7}=text(at,0.2,0.5,s,'Color','b','FontSize',12);
            s=sprintf('r = %f',R);
            h{8}=text(at,0.2,0.4,s,'Color','k','FontSize',12);
            axis(at,'off'); hold(at,'off');
        end

        function tmProcess(app,~,~)
            RsFrameSet = app.RsPipeLine.wait_for_frames();
            if app.SupportDepth ~= 0
               df = RsFrameSet.get_depth_frame();
               dc = app.RsColorizer.colorize(df);
               dd = dc.get_data();
               app.hD.CData = permute(reshape(dd',[3,dc.get_width(),dc.get_height()]),[3 2 1]);
            end
            if app.SupportColor ~= 0
               cf = RsFrameSet.get_color_frame();
               dd = cf.get_data();
               app.hC.CData = permute(reshape(dd',[3,cf.get_width(),cf.get_height()]),[3 2 1]);
            end
            if app.SupportAccel ~= 0
               fa = RsFrameSet.first(realsense.stream.accel);
               ma = fa.as('motion_frame');
               d = ma.get_motion_data();
               app.set_motion_data(app.hA,d);
            end
            if app.SupportGyro ~= 0
               fg = RsFrameSet.first(realsense.stream.gyro);
               mg = fg.as('motion_frame');
               d = mg.get_motion_data();
               app.set_motion_data(app.hG,d);
            end
            drawnow;
            delete(RsFrameSet);
        end

        function onStartButton(app, ~)
            if strcmp(app.tmr.Running, 'off')
               app.RsPipeLine.start();
               start(app.tmr);
            end
        end

        function onStopButton(app,~)
            if strcmp(app.tmr.Running, 'on')
               stop(app.tmr);
               app.RsPipeLine.stop();
            end
        end

        function UIFigureCloseRequest(app,~)
            if strcmp(app.tmr.Running, 'on')
               stop(app.tmr);
               app.RsPipeLine.stop();
            end
            delete(app.tmr);
            delete(app.RsPipeLine);
            delete(app.RsColorizer);
            delete(app);
        end

        function StartUpFunc(app)
            RsCtx = realsense.context();
            Devs = RsCtx.query_devices();
            if ( numel(Devs) < 1 )
                error ( 'Not found RealSense device' );
            end
            RsDev = Devs{1};
            cn = RsDev.get_info(realsense.camera_info.name);
            delete(RsDev);
            delete(RsCtx);
            app.UIFigure.Name = sprintf('rs_capture_class < Camera : %s >',cn);
            app.RsPipeLine = realsense.pipeline();
            app.RsColorizer = realsense.colorizer();
            app.RsPipeLine.start();
            sProf = app.RsPipeLine.get_active_profile();
            Streams = sProf.get_streams();
            delete(sProf);
            kStreams = numel(Streams);
            app.SupportDepth=0;
            app.SupportColor=0;
            app.SupportAccel=0;
            app.SupportGyro=0;
            for i=1:kStreams
                Stream = Streams{i};
                StreamType = Stream.stream_type();
                if strcmp(StreamType, 'depth')
                   app.SupportDepth = 1;
                end
                if strcmp(StreamType, 'color')
                   app.SupportColor = 1;
                end
                if strcmp(StreamType, 'accel')
                   app.SupportAccel = 1;
                end
                if strcmp(StreamType, 'gyro')
                   app.SupportGyro = 1;
                end
                delete(Stream);
            end
            RsFrameSet = app.RsPipeLine.wait_for_frames();
            if app.SupportDepth ~=0
               df = RsFrameSet.get_depth_frame();
               dc = app.RsColorizer.colorize(df);
               dd = dc.get_data();
               d = permute(reshape(dd',[3,dc.get_width(),dc.get_height()]),[3 2 1]);
               app.hD = imshow(d,'Parent',app.DepthAX);
            else
               d = zeros(480,640,3,'uint8'); d(:)=240;
               app.hD = imshow(d,'Parent',app.DepthAX);
               hold(app.DepthAX,'on');
               text(app.DepthAX,140,220,'The <Depth> property is not supported','Color','k','FontSize',16);
               hold(app.DepthAX,'off');
            end
            if app.SupportColor ~= 0
               cf = RsFrameSet.get_color_frame();
               dd = cf.get_data();
               d = permute(reshape(dd',[3,cf.get_width(),cf.get_height()]),[3 2 1]);
               app.hC = imshow(d,'Parent',app.ColorAX);
            else
               d = zeros(480,640,3,'uint8'); d(:)=240;
               app.hC = imshow(d,'Parent',app.ColorAX);
               hold(app.ColorAX,'on');
               text(app.ColorAX,140,220,'The <Color> property is not supported','Color','k','FontSize',16);
               hold(app.ColorAX,'off');
            end
            if app.SupportAccel ~= 0
               fa = RsFrameSet.first(realsense.stream.accel);
               ma = fa.as('motion_frame');
               d = ma.get_motion_data();
               app.hA = app.draw_motion_data(app.AccelAX,app.AccelTextAX,d,'Accel');
            else
               d = [ -0.5, 0.5, sqrt(0.5) ];
               app.hA = app.draw_motion_data(app.AccelAX,app.AccelTextAX,d,'Accel');
               app.hA{5}.String='-------------';
               app.hA{6}.String='-------------';
               app.hA{7}.String='-------------';
               app.hA{8}.String='not supported';
            end
            if app.SupportGyro ~= 0
               fg = RsFrameSet.first(realsense.stream.gyro);
               mg = fg.as('motion_frame');
               d = mg.get_motion_data();
               app.hG = app.draw_motion_data(app.GyroAX,app.GyroTextAX,d,'Gyro');
            else
               d = [ 0.5, -0.5, sqrt(0.5) ];
               app.hG = app.draw_motion_data(app.GyroAX,app.GyroTextAX,d,'Gyro');
               app.hG{5}.String='-------------';
               app.hG{6}.String='-------------';
               app.hG{7}.String='-------------';
               app.hG{8}.String='not supported';
            end
            app.tmr = timer ...
            ( ...
             'ExecutionMode', 'fixedRate', ...    % 'fixedSpacing', ...
             'Period',app.Period, ...
             'BusyMode', 'queue',...              % 'drop', ...
             'TimerFcn',@app.tmProcess ...
            );
            app.RsPipeLine.stop();                            %--- STOP ---
            drawnow;
            delete(RsFrameSet);
        end

        function createComponents(app)
         tVect = 0:pi/60:2*pi;
         app.dV{1} = ((0:1/60:2)-1)*1.15;
         app.dV{2} = sin(tVect);
         app.dV{3} = cos(tVect);
         app.dV{4} = tVect; app.dV{4}(:) = 0;
         kFramePerSecond = 30;  % Number of frames per second
         app.Period = double(int64(1000.0/kFramePerSecond+0.5))/1000.0;
         app.UIFigure = uifigure('Visible', 'off');
         app.UIFigure.Position = [20 20 1310 840];
         app.UIFigure.Name = '< Camera : ? >';
         app.UIFigure.CloseRequestFcn = createCallbackFcn(app,@UIFigureCloseRequest);
         setAutoResize(app,app.UIFigure,false);
         app.DepthAX = uiaxes(app.UIFigure);
         app.DepthAX.Position = [10 40 640 480];
         app.ColorAX = uiaxes(app.UIFigure);
         app.ColorAX.Position = [660 40 640 480];
         app.AccelAX = uiaxes(app.UIFigure);
         app.AccelAX.Position = [30 530 330 300];
         app.AccelTextAX = uiaxes(app.UIFigure);
         app.AccelTextAX.Position = [390 530 240 300];
         app.GyroAX = uiaxes(app.UIFigure);
         app.GyroAX.Position = [680 530 330 300];
         app.GyroTextAX = uiaxes(app.UIFigure);
         app.GyroTextAX.Position = [1040 530 240 300];
         app.StartButton = uibutton(app.UIFigure, 'push');
         app.StartButton.ButtonPushedFcn = createCallbackFcn(app, @onStartButton, true);
         app.StartButton.IconAlignment = 'center';
         app.StartButton.Position = [10 10 100 30];
         app.StartButton.Text = 'Start';
         app.StopButton = uibutton(app.UIFigure, 'push');
         app.StopButton.ButtonPushedFcn = createCallbackFcn(app, @onStopButton, true);
         app.StopButton.IconAlignment = 'center';
         app.StopButton.Position = [120 10 100 30];
         app.StopButton.Text = 'Stop';
         app.UIFigure.Visible = 'on';
         drawnow;
        end
    end

    methods (Access = public)

        function app = capture_example
         createComponents(app)
         registerApp(app, app.UIFigure)
         runStartupFcn(app,@StartUpFunc);
         if nargout == 0
          clear app
         end            
        end

        function delete(app)
         delete(app.UIFigure)
        end
    end
end
