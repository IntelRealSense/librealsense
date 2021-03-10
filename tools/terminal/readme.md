# rs-terminal Tool

## Goal
`rs-terminal` tool can be used to send debug commands to camera firmware. The tool can operate in two modes - if you have access to the specification of debug commands for the exact firmware you are using (provided in XML form) the tool will provide help and auto-completion based on the information within the XML. If you don't have access to the spec, you can still operate the tool in HEX mode, sending binary commands directly. Later mode of operation is intended to allow Intel support team to troubleshoot the hardware remotely by providing the user the exact binary commands to input. 

## Usage
After installing `librealsense` run `rs-terminal` to launch the tool in HEX mode. 

* `-l <filename>` - load commands specification file.
* `-d <number>` - choose device by device number
* `-n <serial>` - choose device by serial number
* `-a` - broadcast to all connected devices



