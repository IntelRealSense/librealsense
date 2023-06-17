// Copyright (c) 2018 Intel Corporation. All rights reserved.
// Use of this source code is governed by an Apache 2.0 license
// that can be found in the LICENSE file.

'use strict';

const ResponseTag = {
  /*
  structure of genericOkType:
  {
    tag: 'ok'
  }
  */
  genericOk: 'ok',
  /*
  structure of errorType:
  {
    tag: 'error',
    description: string description of the error
  }
  */
  error: 'error',
  /*
  structure of presetsType
  {
    tag: 'presets',
    data: array of preset strings
  }
  */
  presets: 'presets',
  /*
  structure of optionsType:
  {
    tag: 'options',
    data: array of sensorOptionType
  }
  structure of sensorOptionType:
  {
    sensor: sensor name,
    options: array of optionType
  }
  structure of optionType:
  {
    option: option name,
    value: current value,
    range: optionRangeType
  }
  structure of optionRangeType:
  {
    minValue: min value
    maxValue: max value
    defaultValue: max value
    step: step
  }
  */
  options: 'options',
  /*
  structure of sensorInfoType
  {
    tag: 'sensor-info',
    data: array of SensorType
  }

  structure of sensorType:
  {
    name: sensor name
    resolutions: array of width and height value paris, for example: [{w1, h1}, {w2, h2}...]
    fpses: array of fps values
    streams: array of streamType
  }

  structure of streamType:
  {
    index: stream index
    name: stream name
    formats: array of supported stream formats
  }
  */
  sensorInfo: 'sensor-info',
  /*
  structure of defaultCfg
  {
    tag: 'default-config',
    data: cfgType
  }
  structure of cfgType
  {
    preset: default preset
    resolution: array of sensor name to resolution, example: ['Stereo Module', '1280*720'],
    fps: array of sensor name to fps, example: ['Stereo Module', 30]
    format: array of stream name to format, example: ['depth', 'z16'],
    streams: array of defalt enabled streams. example ['depth', 'color', 'infrared1']
  }
  */
  defaultCfg: 'default-config',
};

const CommandTag = {
  /*
  structure of setPresetType:
  {
    tag: 'set-preset',
    data: preset string
  }
  */
  setPreset: 'set-preset',
  /*
  structure of setOptionType:
  {
    tag: 'set-option',
    data: {
      sensor: sensor name,
      option: option name,
      value: value
    }
  }
  */
  setOption: 'set-option',
  /*
  structure of startType:
  {
    tag: 'start',
    data: startDataType
  }
  structure of startDataType:
  {
    sensor: sensor name
    streams: array of streamConfigType:
  }
  structure of streamConfigType:
  {
      stream: stream name,
      index: optional, the index the stream.
      resolution: example: w*h
      format: format string,
      fps: fps
    }
  }
  */
  start: 'start',
};

const serverInfo = {
  cmdPort: 3100,
};

const CommonNames = {
  colorSensorName: 'RGB Camera',
  stereoSensorName: 'Stereo Module',
  infraredStreamName: 'infrared',
  colorStreamName: 'color',
  infraredStream1Name: 'infrared1',
  infraredStream2Name: 'infrared2',
  stereoStreamName: 'depth',
};

if (typeof module !== 'undefined' && module.exports) {
  module.exports = {
    ResponseTag,
    CommandTag,
    CommonNames,
    serverInfo,
  };
}
