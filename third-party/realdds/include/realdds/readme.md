# librealsense2 DDS topics

## Main idea
`librealsense2` can discover RealSense devices connected through ethernet by listening to a set of DDS messages.

## Topics list

### realsense/devices

| field         | details                                        |
| ------------- | ---------------------------------------------- |
| name          | device name                                    |
| serial_number | device serial number                           |
| product_line  | device product line (D4xx/L5xx ...)            |
| locked        | true if device flash is locked, false otherwise |

## DDS topic creation / update
* Create an IDL file (See [here](https://fast-dds.docs.eprosima.com/en/latest/fastddsgen/dataTypes/dataTypes.html) )
* Generate C++ files to define the topic data structure and add a serialize/de-serialize capabilities using [FastDDSGen](https://fast-dds.docs.eprosima.com/en/latest/fastddsgen/introduction/introduction.html#fastddsgen-intro)
* Rename cxx generated files into cpp files
* `librealsense2` idl and auto generated files will need to include Intel's copyrights notice on top
