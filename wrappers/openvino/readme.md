# OpenVINO Samples for Intel® RealSense™ cameras
Examples in this folder are designed to complement existing
[SDK examples](../../examples) and demonstrate how Intel RealSense cameras can
be used together with the OpenVINO™ toolkit in the domain of computer-vision.

> RealSense examples have been designed and tested with OpenVINO version
> 2019.3.379. Working with newer versions may require code changes.

## List of Samples:
1. [Face](./face) - Facial recognition

## Getting Started:
Before attempting any installation of OpenVINO, it is highly recommended that
you check the latest [OpenVINO Getting Started guide](https://docs.openvinotoolkit.org/latest/index.html). This guide is in no way comprehensive.

* [Windows Installation](#windows)
* [Linux Installation](#linux)

These samples also require OpenCV, though this is not an OpenVINO requirement.
The OpenCV installation (or where it was built, e.g. `C:/work/opencv/build`)
should be pointed to using a CMake `OpenCV_DIR` entry. Please refer to
[OpenCV samples](../opencv) for additional information.
> NOTE: You can use the OpenCV that is packaged with OpenVINO if you don't want
> to bother with installing it yourself. Add `-DOpenCV_DIR=<openvino-dir>/opencv/cmake`
> to your CMake command-line.

### Windows

See the Windows-specific OpenVINO guide [here](https://docs.openvinotoolkit.org/latest/_docs_install_guides_installing_openvino_windows.html).

> NOTE: the code here will change for you and is given only as an example.
> Your experience may vary!

After downloading and installing OpenVINO in `C:\work\openvino\openvino_2019.3.379`:

1. Open a shell window
```bash
>  cd C:\work\openvino\openvino_2019.3.379
```

2. Make sure you have Python and its scripts, e.g. `pip`, installed and on your
path, otherwise some failures may be encountered:
```bash
>  set "PATH=%PATH%;C:/Program Files (x86)/Microsoft Visual Studio/Shared/Python36_64;C:\Program Files (x86)\Microsoft Visual Studio\Shared\Python36_64\Scripts"
>  bin\setupvars.bat
```

3. Install prerequisites
```bash
>  cd deployment_tools\model_optimizer\install_prerequisites
>  install_prerequisites.bat
...
```
 > NOTE: We ran into issues with the `networkx` library because of API changes.
 > When manually revering back to version `1.11` everything worked.

 > NOTE: If you have to redo this step or the following, you may want to clean
 > any previously-installed packages. By default, OpenVINO installs these under
 > `C:\Users\<user>\Documents\Intel\OpenVINO`.

4. Run the demo scripts and perform additional configuration steps:
```bash
>  cd ../../demo
>  demo_squeezenet_download_convert_run.bat
...
>  demo_security_barrier_camera.bat
...
```

5. Download models
```bash
>  python tools/model_downloader/downloader.py --all --output_dir C:\Users\<user>\Documents\Intel\OpenVINO\models
```
 This will take a while.

Please refer to the example `CMakeLists.txt` files provided in this directory
and the individual samples to see how to properly integrate with OpenVINO.

### Runtime Dependencies

Many of the examples require DLL files or collaterals that can be found. Rather
than specify a hard-coded path, they usually expect these to be available in the
same directory as the .exe. If the .exe is at `c:/work/lrs/build/Release` then
place any missing DLLs or models in the same directory, or change the code to
look in a different place.


### Linux

1. Download the latest OpenVINO toolkit and follow the [instructions for Linux installation](https://docs.openvinotoolkit.org/latest/_docs_install_guides_installing_openvino_linux.html). These are very similar to
the Windows instructions above.
2. Follow [the instructions](https://github.com/IntelRealSense/librealsense/blob/master/doc/installation.md) to build `librealsense` from source, but:
 * Add `-DBUILD_OPENVINO_EXAMPLES=true -DOpenCV_DIR=... -DINTEL_OPENVINO_DIR=...` to your `cmake` command
3. Make sure you have `$LD_LIBRARY_PATH` pointing to the OpenVINO libraries when
 running the examples. Run `source $INTEL_OPENVINO_DIR/bin/setupvars.sh` to do
 this.
