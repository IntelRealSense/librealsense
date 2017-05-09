# Troubleshooting Tips and Tricks

## Kernel Version
- Retrieve Kernel Version
```bash
$ uname -r
```

## Enable LibRealSense Log
- In order to change the log level of LibRealSense logger, you need to set a local variable named LRS_LOG_LEVEL and initialize it with the desirable log level

On Linux:
```bash
$ export LRS_LOG_LEVEL="<Log Level>"
```
On Windows:
```bash
$ set LRS_LOG_LEVEL="<Log Level>"
```
- A LibRealSense log will be created even an application didn't activate LibRealSense logger.

## Connected Intel Cameras
- List of all connected Intel cameras
```bash
$ lsusb | grep 8086
```

## General Linux Kernel Log
- Retrieve last Linux Kernel log messages with timestamps
```bash
$ dmesg -T
```

- Clear dmesg buffer
```bash
$ sudo dmesg -c
```

- Linux writes all OS logs to ```/var/log``` folder. In order to review the whole kernel log file, use below line
```bash
$ less /var/log/kern.log
```

## UVC Video Module Traces
You can get more verbose logs from uvcvideo kernel-module, these logs can be seen in `dmesg`. 
- Enable UVC driver verbose log
```bash
$ sudo echo 0xFFFF > /sys/module/uvcvideo/parameters/trace
```
- In order to disable UVC verbose log replace 0xFFFF with 0.

For example, once enabled you will get the following line inside `dmesg` for each frame received from USB: 
```bash
[619003.810541] uvcvideo: frame 1 stats: 0/0/1 packets, 0/0/1 pts (!early initial), 0/1 scr, last pts/stc/sof 25177741/25178007/81
[619003.810546] uvcvideo: Frame complete (FID bit toggled).
[619003.810556] uvcvideo: frame 2 stats: 0/0/1 packets, 0/0/1 pts (!early initial), 0/1 scr, last pts/stc/sof 25210903/25211168/346
[619003.810588] uvcvideo: uvc_v4l2_poll
[619003.811173] uvcvideo: uvc_v4l2_poll
[619003.843768] uvcvideo: frame 3 stats: 0/0/1 packets, 0/0/1 pts (!early initial), 0/1 scr, last pts/stc/sof 25210903/25211168/346
[619003.843774] uvcvideo: Frame complete (FID bit toggled).
[619003.843785] uvcvideo: frame 4 stats: 0/0/1 packets, 0/0/1 pts (!early initial), 0/1 scr, last pts/stc/sof 25244064/25244330/612
```

## Kernel Events
- Listening to camera connect/disconnect events
```bash
$ sudo udevadm monitor
```

## System Calls and Signals
- To get a verbose log of all calls an application makes to the kernel, run the application under `strace`:
```bash
$ strace <Application Path>
```

## Core Dump File
In case of a crash (for example SEGFAULT) a snapshot of the crash can be created (Core Dump) and submitted for inspection. 
- Allow auto-creation of Core Dump files:
```bash
$ ulimit -c unlimited
```
- Run the crashing application
- Search for `core` file in the current directory

**Pay attention that auto-creation of dump file will works only on the same Terminal you ran the ulimit command.**