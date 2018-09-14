# Getting Started with RealSense™ SDK2.0 for Matlab®

## Introduction:
The latest generation of RealSense Depth Cameras can be controlled via the RealSense SDK2.0, also referred to as libRealSense 2.x.x, or LibRS for short. This includes stereo depth cameras D415 and D435 as well as their corresponding modules, such as D410 and D430.

To get started controlling the RealSense Cameras with Matlab® in Windows 10, we have created a package that wraps some of the core functions of the realsense2.dll, and we have also created a simple “Hello World” function to get started capturing depth data. This uses Matlab  R2017b and requires a Windows 10 laptop with a USB3 port.

## Getting Started
### Building from Source
1. Download the Git repository and run CMake, specifying the x64 toolchain and static linkage (`-DBUILD_SHARED_LIBS:BOOL=OFF`).
2. Open Visual Studio and import the project file located [here](./librealsense_mex.vcxproj).
3. Make sure that the project settings are consistent with the instructions listed in this web page for [Compiling Mex File with Visual Studio](https://www.ini.uzh.ch/~ppyk/BasicsOfInstrumentation/matlab_help/matlab_external/compiling-mex-files-with-the-microsoft-visual-c-ide.html) and your system. For more information on Matlab settings, you can also refer to this [Matlab example project](http://coachk.cs.ucf.edu/GPGPU/Compiling_a_MEX_file_with_Visual_Studio2.htm).
4. Make sure that in the project's settings `Target Platform Version` and `Platform Toolset` match the values set for the `realsense2` target.
5. Build the `realsense2` target.
6. Build the `librealsense_mex` target.
7. After compiling the project, set [build/Debug](../../build/Debug) or [build/Release](../../build/Release) as your Matlab working directory. Alternatively copy the `+realsense` folder from there to a place where Matlab can find it.
8. Our simple example can be run by typing [`realsense.depth_example`](./depth_example.m) at the Matlab prompt.

### Windows Installer
1. Run the Windows Installer and select the Matlab Developer Package checkbox.

    ![Image of Installer](../../doc/img/matlab_select.png)
2. Allow the installer to complete
3. The package will be installed to `C:\Program Files (x86)\Intel RealSense SDK 2.0\matlab\+realsense\`. You can copy it from here to a place where Matlab can find it or add it to Matlab's path

## Examples
#### Displaying a frame using _realsense.pipeline_
```Matlab
function depth_example()
    % Make Pipeline object to manage streaming
    pipe = realsense.pipeline();
    % Make Colorizer object to prettify depth output
    colorizer = realsense.colorizer();

    % Start streaming on an arbitrary camera with default settings
    profile = pipe.start();

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
    channels = vec2mat(data, 3);
    img(:,:,1) = vec2mat(channels(:,1), color.get_width());
    img(:,:,2) = vec2mat(channels(:,2), color.get_width());
    img(:,:,3) = vec2mat(channels(:,3), color.get_width());

    % Display image
    imshow(img);
    title(sprintf("Colorized depth frame from %s", name));
end
```
