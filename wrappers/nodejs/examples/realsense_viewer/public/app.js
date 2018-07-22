// Copyright (c) 2018 Intel Corporation. All rights reserved.
// Use of this source code is governed by an Apache 2.0 license
// that can be found in the LICENSE file.

/* globals Blob, CommandTag, CommonNames, Image, ResponseTag, URL, Vue, WebSocket, document,
 serverInfo, window */

'use strict';

class RealsenseProxy {
  constructor(cmdPort, callback, dataCallbacks) {
    this.url = 'ws://' + window.location.hostname + ':' +cmdPort;
    this.cmdSocket = null;
    this.cmdSocketReady = false;
    this.dataSockets = new Map();
    this.dataSocketReady = new Map();
    this.responseCallback = callback;
    this.dataCallbacks = dataCallbacks;
  }
  connect() {
    this.cmdSocket = new WebSocket(this.url);
    this.cmdSocket.addEventListener('open', (event) => {
      this.cmdSocketReady = true;
    });
    this.cmdSocket.addEventListener('message', (event) => {
      this.parseMessage(event.data);
    });
    for (let stream of [
        CommonNames.colorStreamName,
        CommonNames.stereoStreamName,
        CommonNames.infraredStream1Name,
        CommonNames.infraredStream2Name]) {
      this._setupConnectionCallback(stream);
    }
  }
  startStreaming(sensor, selection) {
    let cmd = {
      tag: CommandTag.start,
      data: {
        sensor: sensor,
        streams: [],
      },
    };
    if (sensor === CommonNames.colorSensorName) {
      if (selection.selectedStreams[CommonNames.colorStreamName]) {
        let stream = {
          stream: CommonNames.colorStreamName,
          resolution: selection.selectedResolutions[CommonNames.colorSensorName],
          format: selection.selectedFormats[CommonNames.colorStreamName],
          fps: selection.selectedFpses[CommonNames.colorSensorName],
        };
        cmd.data.streams.push(stream);
      } else {
        console.error('color stream not selected!');
      }
    } else if (sensor === CommonNames.stereoSensorName) {
      let res = selection.selectedResolutions[CommonNames.stereoSensorName];
      let fps = selection.selectedFpses[CommonNames.stereoSensorName];
      if (selection.selectedStreams[CommonNames.infraredStream1Name]) {
        let stream = {
          stream: CommonNames.infraredStreamName,
          resolution: res,
          index: 1,
          format: selection.selectedFormats[CommonNames.infraredStream1Name],
          fps: fps,
        };
        cmd.data.streams.push(stream);
      }
      if (selection.selectedStreams[CommonNames.infraredStream2Name]) {
        let stream = {
          stream: CommonNames.infraredStreamName,
          resolution: res,
          index: 2,
          format: selection.selectedFormats[CommonNames.infraredStream2Name],
          fps: fps,
        };
        cmd.data.streams.push(stream);
      }
      if (selection.selectedStreams[CommonNames.stereoStreamName]) {
        let stream = {
          stream: CommonNames.stereoStreamName,
          resolution: res,
          format: selection.selectedFormats[CommonNames.stereoStreamName],
          fps: fps,
        };
        cmd.data.streams.push(stream);
      }
    }
    console.log('send start cmd:');
    this._sendCmd(JSON.stringify(cmd));
  }
  _setupConnectionCallback(streamVal) {
    let inst = this;
    this.dataSockets[streamVal] = new WebSocket(this.url);
    this.dataSockets[streamVal].addEventListener('open', function(event) {
      console.log(`${streamVal} data connection connected`);
      inst.dataSocketReady[streamVal] = true;
      inst.dataSockets[streamVal].send(JSON.stringify({stream: streamVal}));
    });
    this.dataSockets[streamVal].addEventListener('message', function(event) {
      inst.parseDataMessage(streamVal, event.data);
    });
  }
  _sendCmd(cmdString) {
    if (!this.cmdSocketReady) {
      throw new Error('command channel not connected!');
    }
    this.cmdSocket.send(cmdString);
  }
  parseMessage(msgString) {
    console.log('received message:', msgString);
    this.responseCallback(JSON.parse(msgString));
  }
  parseDataMessage(stream, dataMsg) {
    if (typeof dataMsg === 'string') {
      // received data meta string
    } else {
      // received raw data
      this.dataCallbacks[stream](dataMsg);
    }
  }
}
const dataCallbacks = {
  'color': colorDataCallback,
  'depth': depthDataCallback,
  'infrared1': infrared1DataCallback,
  'infrared2': infrared2DataCallback,
};

const proxy = new RealsenseProxy(serverInfo.cmdPort, cmdResponseCallback, dataCallbacks);
const vue = new Vue({
  el: '#web-demo',
  data: {
    sensorInfo: [],
    sensorInfoStr: '',
    presets: [],
    options: '',
    selection: {
      selectedPreset: '',
      selectedResolutions: {},
      selectedFpses: {},
      selectedStreams: {},
      selectedFormats: {},
    },
    defaultCfg: {},
    canvasData: {
      started: new Map(),
      width: {
        'color': 0,
        'depth': 0,
        'infrared1': 0,
        'infrared2': 0,
      },
      height: {
        'color': 0,
        'depth': 0,
        'infrared1': 0,
        'infrared2': 0,
      },
      display: {
        'color': false,
        'depth': false,
        'infrared1': false,
        'infrared2': false,
      },
    },
  },
  mounted: function() {
    proxy.connect();
  },
  methods: {
    onstartStereo: function(event) {
      vue.canvasData.started.set(CommonNames.stereoStreamName, true);
      let size = resolutionStringToNumberPair(
          vue.selection.selectedResolutions[CommonNames.stereoSensorName]);
      vue.canvasData.width[CommonNames.stereoStreamName] = size.width;
      vue.canvasData.height[CommonNames.stereoStreamName] = size.height;
      vue.canvasData.display[CommonNames.stereoStreamName] = true;

      updateCanvasConfig();
      proxy.startStreaming(CommonNames.stereoSensorName, vue.selection);
      console.log('selected Resolutions:', vue.selection.selectedResolutions);
      console.log('selected Fpses:', vue.selection.selectedFpses);
      console.log('selected Streams:', vue.selection.selectedStreams);
      console.log('selected Formats:', vue.selection.selectedFormats);
      console.log('selected Preset:', vue.selection.selectedPreset);
    },
    onstartColor: function(event) {
      vue.canvasData.started.set(CommonNames.colorStreamName, true);
      let size = resolutionStringToNumberPair(
          vue.selection.selectedResolutions[CommonNames.colorSensorName]);
      vue.canvasData.width[CommonNames.colorStreamName] = size.width;
      vue.canvasData.height[CommonNames.colorStreamName] = size.height;
      vue.canvasData.display[CommonNames.colorStreamName] = true;
      updateCanvasConfig();
      proxy.startStreaming(CommonNames.colorSensorName, vue.selection);
      console.log('selected Resolutions:', vue.selection.selectedResolutions);
      console.log('selected Fpses:', vue.selection.selectedFpses);
      console.log('selected Streams:', vue.selection.selectedStreams);
      console.log('selected Formats:', vue.selection.selectedFormats);
      console.log('selected Preset:', vue.selection.selectedPreset);
    },
  },
});

function cmdResponseCallback(response) {
  console.log(response);
  switch (response.tag) {
    case ResponseTag.presets:
      vue.presets = response.data;
      break;
    case ResponseTag.options:
      vue.options = JSON.stringify(response);
      break;
    case ResponseTag.sensorInfo:
      vue.sensorInfo = response.data;
      break;
    case ResponseTag.defaultCfg:
      vue.defaultCfg = response.data;
      vue.selection.selectedPreset = vue.defaultCfg.preset;
      vue.defaultCfg.resolution.forEach((pair) => {
        vue.selection.selectedResolutions[pair[0]] = pair[1];
      });
      vue.defaultCfg.fps.forEach((pair) => {
        vue.selection.selectedFpses[pair[0]] = pair[1];
      });
      vue.defaultCfg.format.forEach((pair) => {
        vue.selection.selectedFormats[pair[0]] = pair[1];
      });
      vue.defaultCfg.streams.forEach((s) => {
        vue.selection.selectedStreams[s] = true;
      });
      break;
    default:
      break;
  }
}

function colorDataCallback(data) {
  let blob = new Blob([data], {type: 'image/jpg'});
  let url = URL.createObjectURL(blob);
  let img = new Image();
  img.src = url;
  let canvas = document.getElementById('color-canvas');
  let ctx = canvas.getContext('2d');
  img.onload = function() {
    ctx.drawImage(this, 0, 0);
    URL.revokeObjectURL(url);
  };
}

function depthDataCallback(data) {
  let blob = new Blob([data], {type: 'image/jpg'});
  let url = URL.createObjectURL(blob);
  let img = new Image();
  img.src = url;
  let canvas = document.getElementById('depth-canvas');
  let ctx = canvas.getContext('2d');
  img.onload = function() {
    ctx.drawImage(this, 0, 0);
    URL.revokeObjectURL(url);
  };
}

function infrared1DataCallback(data) {
  // todo(tingshao): Add infrared support
}
function infrared2DataCallback(data) {
  // todo(tingshao): Add infrared support
}
function resolutionStringToNumberPair(resolution) {
  let pair = resolution.split('*').map((x) => Number(x));
  return {width: pair[0], height: pair[1]};
}

function updateCanvasConfig() {
  let startedCount = 0;
  vue.canvasData.started.forEach((val, key) => {
    if (val) {
      startedCount++;
    }
  });
  if (startedCount >= 2) {
    vue.canvasData.started.forEach((val, key) => {
      if (val) {
        let oldWidth = vue.canvasData.width[key];
        let oldHeight = vue.canvasData.height[key];

        vue.canvasData.width[key] = oldWidth/2;
        vue.canvasData.height[key] = oldHeight/2;
      }
    });
  }
}
