# rs-convert Tool

## Goal

Console app for converting ROS-bag files to various formats (currently supported: PNG, RAW, CSV, PLY, BIN)

## Command Line Parameters

|Flag   |Description   |Default|
|---|---|---|
|`-i <ros-bag-file>`|ROS-bag filename||
|`-p <png-path>`|convert to PNG, set output path to <png-path>||
|`-v <csv-path>`|convert to CSV (depth matrix), set output path to <csv-path>||
|`-r <raw-path>`|convert to RAW, set output path to <raw-path>||
|`-l <ply-path>`|convert to PLY, set output path to <ply-path>||
|`-b <bin-path>`|convert to BIN (depth matrix), set output path to <bin-path>||
|`-d`|convert depth frames only||
|`-c`|convert color frames only||

## Usage

**Example**: If you have `1.bag` recorded from the Viewer or from API, copy it next to `rs-convert.exe` (for Windows), launch the command line and enter: `rs-convert.exe -v test -i 1.bag`. This will generate one `.csv` file for each frame inside the `.bag` file. 

Several converters can be used simultaneously, e.g.:
`rs-convert -i some.bag -p some_dir/some_file_prefix -r some_another_dir/some_another_file_prefix`
