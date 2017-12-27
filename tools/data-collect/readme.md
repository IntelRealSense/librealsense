# rs-data-collect Tool

## Goal

This tool is designed to help collect statistics about various streams.

## Command Line Parameters

|Flag   |Description   |Default|
|---|---|---|
|`-c <filename>`|Load stream configuration from <filename>||
|`-m X`|Stop the test after receiving at least X frames|100|
|`-t X`|Stop the test after X seconds|10|
|`-f <filename>`|Save results into <filename>||
  
### Config File Format
```
STREAM1,WIDTH1,HEIGHT1,FPS1,FORMAT1,
STREAM2,WIDTH2,HEIGHT2,FPS2,FORMAT2,
...
```
  
## Usage
Provided a configuration file the tool will configure the device to the desired streams and wait for either set number of frames or a timeout (whatever comes first). For each frame, timestamp, frame-number, time of arrival, stream profile, and timestamp domain will be recorded into the output log. **The tool does not record the content of the frames**
