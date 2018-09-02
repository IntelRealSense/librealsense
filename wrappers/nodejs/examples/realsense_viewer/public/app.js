// Copyright (c) 2018 Intel Corporation. All rights reserved.
// Use of this source code is governed by an Apache 2.0 license
// that can be found in the LICENSE file.

/* globals Blob, CommandTag, CommonNames, Image, ResponseTag, URL, Vue, WebSocket, document,
 serverInfo, window, FileReader, alert */

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
  setOption(sensor, option, value) {
    let cmd = {
      tag: CommandTag.setOption,
      data: {
        sensor: sensor,
        option: option,
        value: value,
      },
    };
    console.log('send set option cmd:');
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
    this.responseCallback(JSON.parse(msgString));
  }
  parseDataMessage(stream, dataMsg) {
    if (typeof dataMsg === 'string') {
      let obj = JSON.parse(dataMsg);
      if (obj) {
        if (stream === CommonNames.infraredStream1Name) {
          glData.textureWidth[0] = obj.width;
          glData.textureHeight[0] = obj.height;
        } else if (stream === CommonNames.infraredStream2Name) {
          glData.textureWidth[1] = obj.width;
          glData.textureHeight[1] = obj.height;
        }
      }
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
    controls: {
      enabled: {},
      options: {},
    },
  },
  mounted: function() {
    proxy.connect();
  },
  methods: {
    onstartStereo: function(event) {
      let size = resolutionStringToNumberPair(
          vue.selection.selectedResolutions[CommonNames.stereoSensorName]);
      let streams = [CommonNames.stereoStreamName,
          CommonNames.infraredStream1Name,
          CommonNames.infraredStream2Name];
      streams.forEach((stream) => {
        if (vue.selection.selectedStreams[stream]) {
          vue.canvasData.started.set(stream, true);
          vue.canvasData.width[stream] = size.width;
          vue.canvasData.height[stream] = size.height;
          vue.canvasData.display[stream] = true;
        }
      });

      updateCanvasConfig();
      initGL();
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
    onOptionChanged: function(sensorName, optionName, value) {
      proxy.setOption(sensorName, optionName, value);
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
      vue.controls.options =response.data;
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
  let fileReader = new FileReader();
  fileReader.onload = function(event) {
    const arrayBuffer = event.target.result;
    drawInfraredToCanvas(0, arrayBuffer, {
        width: glData.textureWidth[0], height: glData.textureHeight[0],
    });
  };
  fileReader.readAsArrayBuffer(data);
}
function infrared2DataCallback(data) {
  let fileReader = new FileReader();
  fileReader.onload = function(event) {
    const arrayBuffer = event.target.result;
    drawInfraredToCanvas(1, arrayBuffer, {
        width: glData.textureWidth[1], height: glData.textureHeight[1],
    });
  };
  fileReader.readAsArrayBuffer(data);
}

function drawInfraredToCanvas(index, arrayBuffer, textureSize) {
  const gl = glData.infraredGl[index];
  const texture = glData.texture[index];
  gl.bindTexture(gl.TEXTURE_2D, texture);
  gl.texImage2D(gl.TEXTURE_2D, 0, gl.LUMINANCE, textureSize.width,
      textureSize.height, 0, gl.LUMINANCE, gl.UNSIGNED_BYTE,
      new Uint8Array(arrayBuffer));
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
  drawScene(index, {
    width: index === 0 ? vue.canvasData.width.infrared1 : vue.canvasData.width.infrared2,
    height: index === 0 ? vue.canvasData.height.infrared1 : vue.canvasData.height.infrared2,
  });
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

let glData = {
  infraredCanvas: [null, null],
  infraredGl: [null, null],
  texture: [null, null],
  shader: [null, null],
  buffers: [null, null],
  textureWidth: [0, 0],
  textureHeight: [0, 0],
};

function initShaders(gl) {
  const vsSource = `
    attribute vec4 aVertexPosition;
    attribute vec2 aTextureCoord;
    varying highp vec2 vTextureCoord;

    void main(void) {
      gl_Position = aVertexPosition;
      vTextureCoord = aTextureCoord;
    }
  `;
  const fsSource = `
    varying highp vec2 vTextureCoord;
    uniform sampler2D uSampler;

    void main(void) {
      gl_FragColor = texture2D(uSampler, vTextureCoord);
    }
  `;
  const vs = gl.createShader(gl.VERTEX_SHADER);
  const fs = gl.createShader(gl.FRAGMENT_SHADER);
  gl.shaderSource(vs, vsSource);
  gl.shaderSource(fs, fsSource);
  gl.compileShader(vs);
  gl.compileShader(fs);

  if (!gl.getShaderParameter(vs, gl.COMPILE_STATUS)) {
    alert('An error occurred compiling the shaders: ' + gl.getShaderInfoLog(vs));
    gl.deleteShader(vs);
    return null;
  }
  if (!gl.getShaderParameter(fs, gl.COMPILE_STATUS)) {
    alert('An error occurred compiling the shaders: ' + gl.getShaderInfoLog(fs));
    gl.deleteShader(fs);
    return null;
  }

  const shaderProgram = gl.createProgram();
  gl.attachShader(shaderProgram, vs);
  gl.attachShader(shaderProgram, fs);
  gl.linkProgram(shaderProgram);

  if (!gl.getProgramParameter(shaderProgram, gl.LINK_STATUS)) {
    alert('Unable to initialize the shader program: ' + gl.getProgramInfoLog(shaderProgram));
    return null;
  }

  return {
    program: shaderProgram,
    attributeLocations: {
      vertexPosition: gl.getAttribLocation(shaderProgram, 'aVertexPosition'),
      textureCoord: gl.getAttribLocation(shaderProgram, 'aTextureCoord'),
    },
    uniformLocations: {
      uSampler: gl.getUniformLocation(shaderProgram, 'uSampler'),
    },
  };
}

function initBuffers(gl) {
  const positionBuffer = gl.createBuffer();
  gl.bindBuffer(gl.ARRAY_BUFFER, positionBuffer);
  const positions = [
    -1.0, 1.0,
     1.0, 1.0,
    -1.0, -1.0,
     1.0, -1.0,
  ];
  gl.bufferData(gl.ARRAY_BUFFER,
                new Float32Array(positions),
                gl.STATIC_DRAW);

  const textureCoordBuffer = gl.createBuffer();
  gl.bindBuffer(gl.ARRAY_BUFFER, textureCoordBuffer);

  const textureCoordinates = [
    // Front
    0.0, 0.0,
    1.0, 0.0,
    0.0, 1.0,
    1.0, 1.0,
  ];

  gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(textureCoordinates),
      gl.STATIC_DRAW);
  return {
    position: positionBuffer,
    textureCoord: textureCoordBuffer,
  };
}

function initGL() {
  glData.infraredCanvas[0] = document.getElementById('infrared1-canvas');
  glData.infraredCanvas[1] = document.getElementById('infrared2-canvas');
  glData.infraredGl[0] = glData.infraredCanvas[0].getContext('webgl');
  glData.infraredGl[1] = glData.infraredCanvas[1].getContext('webgl');
  glData.texture[0] = glData.infraredGl[0].createTexture();
  glData.texture[1] = glData.infraredGl[1].createTexture();
  glData.shader[0] = initShaders(glData.infraredGl[0]);
  glData.shader[1] = initShaders(glData.infraredGl[1]);
  glData.buffers[0] = initBuffers(glData.infraredGl[0]);
  glData.buffers[1] = initBuffers(glData.infraredGl[1]);
}

function drawScene(index, viewPortSize) {
  const gl = glData.infraredGl[index];
  gl.viewport(0, 0, viewPortSize.width, viewPortSize.height);

  {
    const numComponents = 2;
    const type = gl.FLOAT;
    const normalize = false;
    const stride = 0;
    const offset = 0;
    gl.bindBuffer(gl.ARRAY_BUFFER, glData.buffers[index].position);
    gl.vertexAttribPointer(
        glData.shader[index].attributeLocations.vertexPosition,
        numComponents,
        type,
        normalize,
        stride,
        offset);
    gl.enableVertexAttribArray(
        glData.shader[index].attributeLocations.vertexPosition);
  }

  {
    const num = 2; // every coordinate composed of 2 values
    const type = gl.FLOAT;
    const normalize = false; // don't normalize
    const stride = 0; // how many bytes to get from one set to the next
    const offset = 0; // how many bytes inside the buffer to start from

    gl.bindBuffer(gl.ARRAY_BUFFER, glData.buffers[index].textureCoord);
    gl.vertexAttribPointer(glData.shader[index].attributeLocations.textureCoord, num, type,
        normalize, stride, offset);
    gl.enableVertexAttribArray(glData.shader[index].attributeLocations.textureCoord);
  }

  gl.useProgram(glData.shader[index].program);
  // Tell WebGL we want to affect texture unit 0
  gl.activeTexture(gl.TEXTURE0);
  // Tell the shader we bound the texture to texture unit 0
  gl.uniform1i(glData.shader[index].uniformLocations.uSampler, 0);
  const offset = 0;
  const vertexCount = 4;
  gl.bindBuffer(gl.ARRAY_BUFFER, glData.buffers[index].position);
  gl.drawArrays(gl.TRIANGLE_STRIP, offset, vertexCount);
}
