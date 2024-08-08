# rs-embed Tool

## Goal

The rs-embed tool is used for embedding binary resources into code,  it is an effective solution for distributing 3D-models, raster graphics and fonts. 

This tool offers a standard way this can be done across the SDK.

## Description
This tool can embed 3d object / png files into code by generating a header file with all static data.
a common use case which use it inside libRealSense is rendering the 3D model on the [realsense-viewer tool](../realsense-viewer).

### 3D object file
The tool will read the input obj file, compress it using LZ4 compression tool and create a header file with the compressed data and an `uncompress(...)` method

### PNG file
The tool will read the input png file, and create a header file with the compressed data array and size

## Command Line Parameters

|Flag   |Description   |Default|
|---|---|---|
|`-i <input-file>`|png / obj file that we want to embed||
|`-o <output-file>`|The desired output file path||
|`-n <object-name>`|The embedded object name for the array/function name created||
|`--version`|Get the tool version string||

For example:  
`rs-embed.exe -n d435 -o d435.h -i "inputs/d435.obj"`  
Will create a header for the 3D model of the d435 obj file given as input.
The array name that will be generated is `static uint32_t d435_obj_data [] {...}`
and the decompress function signature will be `inline void uncompress_d435_obj(...)`.

## How to embed a RealSense device 3D model into code.
Here we will learn how to generate an embed a 3D model into code.

1. Download the requested CAD model from the [RealSense website](https://dev.intelrealsense.com/docs/cad-files).
2. Convert the `step` file into an `obj` file. (Can be done using [freecad](https://wiki.freecadweb.org/Download) application)
3. Optional - Reduce object size by narrowing data (Can be done using [meshmixer](https://www.meshmixer.com/download.html) application)
   	* [Here](https://i.materialise.com/blog/en/reduce-the-file-size-of-stl-and-obj-3d-models/) is a guide on how to reduce obj size using meshmixer application.
 4. Run rs-embed tool with the generated object as input.
