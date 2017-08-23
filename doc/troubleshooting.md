# Troubleshooting Tips and Tricks

The following commands can be used to isolate and analyze issues:

## Kernel Version
- To Retrieve the Kernel Version use:
```bash
$ uname -r
```

## Enable LibRealSense Log
- To change the log level of LibRealSense logger, you need to set a local variable named **LRS_LOG_LEVEL**
and initialize it with the desirable log level:

On Linux:
```bash
$ export LRS_LOG_LEVEL="<Log Level>"
```
On Windows:
```bash
$ set LRS_LOG_LEVEL="<Log Level>"
```
- A LibRealSense log will be created even when an application does not activate the LibRealSense logger.

## Connected Intel Cameras
- To list all connected Intel Cameras:
```bash
$ lsusb | grep 8086
```

## General Linux Kernel Log
- To retrieve the last Linux Kernel log messages with timestamps:
```bash
$ dmesg -T
```

- To clear the dmesg buffer:
```bash
$ sudo dmesg -c
```

- Linux writes all OS logs to ```/var/log``` folder.  
To review the entire kernel log file, use:
```bash
$ less /var/log/kern.log
```

## UVC Video Module Traces
You can get more verbose logs from the uvcvideo kernel-module.  
These logs can be seen in `dmesg`
- To enable the UVC driver verbose log:
```bash
$ sudo echo 0xFFFF > /sys/module/uvcvideo/parameters/trace
```
- To disable the UVC verbose log, replace 0xFFFF with 0.

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
- To listen to camera connect/disconnect events:
```bash
$ sudo udevadm monitor
```

## System Calls and Signals
- To get a verbose log of all calls an application makes to the kernel, run the application under `strace`:
```bash
$ strace <Application Path>
```

## Core Dump File
In case of a crash (for example SEGFAULT), a snapshot of the crash can be created (Core Dump) and submitted for inspection.
1. Enable the auto-creation of Core Dump files:
```bash
$ ulimit -c unlimited
```
2. Run the application that causes the crash

3. Search for the `core` file in the current directory

**Note:** Auto-creation of the dump file will only work on the same Terminal that you ran the **ulimit** command.
