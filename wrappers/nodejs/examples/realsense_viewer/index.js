// Copyright (c) 2018 Intel Corporation. All rights reserved.
// Use of this source code is governed by an Apache 2.0 license
// that can be found in the LICENSE file.

'use strict';

const {ResponseTag, CommandTag, CommonNames} = require('./public/common.js');
const WebSocket = require('ws');
const express = require('express');
const path = require('path');
const rsWrapper = require('../../index.js');
const sharp = require('sharp');

const port = 3000;
const wsPort = 3100;
const jpegQuality = 40;

class Realsense {
  constructor(rsWrapper, connectMgr) {
    this.wrapper = rsWrapper;
    this.connectMgr = connectMgr;
    this.sendCount = {};
    for (let name of [
        CommonNames.colorStreamName,
        CommonNames.stereoStreamName,
        CommonNames.infraredStream1Name,
        CommonNames.infraredStream2Name]) {
      this.sendCount[name] = 0;
    }
  }
  init() {
    this.ctx = new this.wrapper.Context();
    this.colorizer = new this.wrapper.Colorizer();
    this.decimate = new this.wrapper.DecimationFilter();
    this.sensors = this.ctx.querySensors();
  }
  stop() {}
  get isReady() {
    return this.sensors;
  }
  cleanup() {
    this.ctx = null;
    this.sensors = null;
    this.wrapper.cleanup();
  }
  // return the presets response
  getPresets() {
    let presets = {
      tag: ResponseTag.presets,
      data: [],
    };
    for (let p in this.wrapper.rs400_visual_preset) {
      if (typeof this.wrapper.rs400_visual_preset[p] === 'string') {
        presets.data.push(this.wrapper.rs400_visual_preset[p]);
      }
    }
    return presets;
  }
  // return the sensorInfo response
  getAllSensorInfo() {
    if (!this.sensors) {
      return undefined;
    }
    let info = {
      tag: ResponseTag.sensorInfo,
      data: [],
    };
    this.sensors.forEach((s) => {
      info.data.push(this._getSensorInfo(s));
    });
    return info;
  }
  // return the options response
  getOptions() {
    if (!this.sensors) {
      return undefined;
    }
    let options = {
      tag: ResponseTag.options,
      data: {},
    };
    this.sensors.forEach((s) => {
      let sensorName = s.getCameraInfo(this.wrapper.camera_info.camera_info_name);
      options.data[sensorName] = this._getSensorOptions(s);
    });
    return options;
  }
  // return the defaultCfg response
  getDefaultConfig() {
    let cfg = {
      tag: ResponseTag.defaultCfg,
      data: {
        preset: 'custom',
        resolution: [[CommonNames.stereoSensorName, '1280*720'],
            [CommonNames.colorSensorName, '1280*720']],
        fps: [[CommonNames.stereoSensorName, 30], [CommonNames.colorSensorName, 30]],
        format: [[CommonNames.stereoStreamName, 'z16'], [CommonNames.infraredStream1Name, 'y8'],
            [CommonNames.infraredStream2Name, 'y8'], [CommonNames.colorStreamName, 'rgb8']],
        streams: [CommonNames.stereoStreamName, CommonNames.colorStreamName,
            CommonNames.infraredStream1Name],
      },
    };
    return cfg;
  }
  // process commands from browser
  processCommand(cmd) {
    switch (cmd.tag) {
      case CommandTag.start:
        console.log('start command');
        console.log(cmd);
        this._handleStart(cmd);
        break;
      case CommandTag.setOption:
        console.log('Set option command');
        console.log(cmd);
        this._handleSetOption(cmd);
        break;
      default:
        console.log(`Unrecognized command ${cmd}`);
        break;
    }
  }
  _getSensorInfo(sensor) {
    let info = {
      name: sensor.getCameraInfo(this.wrapper.camera_info.camera_info_name),
      resolutions: [],
      fpses: [],
      streams: [],
    };
    let streamMap = new Map();
    let profiles = sensor.getStreamProfiles();
    profiles.forEach((p) => {
      // only cares about video stream profile
      if (!(p instanceof this.wrapper.VideoStreamProfile)) {
        return;
      }

      let found = info.resolutions.find((e) => {
        return (e.w === p.width && e.h === p.height);
      });
      if (!found) {
        info.resolutions.push({w: p.width, h: p.height});
      }
      found = info.fpses.find((fps) => {
        return fps === p.fps;
      });
      if (!found) {
        info.fpses.push(p.fps);
      }

      let streamName = this.wrapper.stream.streamToString(p.streamType);
      let formatName = this.wrapper.format.formatToString(p.format);
      let index = p.streamIndex;
      let key = streamName + index;

      if (!streamMap.has(key)) {
        streamMap.set(key, {
          index: index,
          name: streamName,
          formats: [formatName],
        });
      } else {
        let entry = streamMap.get(key);
        found = entry.formats.find((f) => {
          return f === formatName;
        });
        if (!found) {
          entry.formats.push(formatName);
        }
      }
    });
    streamMap.forEach((val, key) => {
      info.streams.push({
        index: val.index,
        name: val.name,
        formats: val.formats,
      });
    });
    return info;
  }
  _getSensorOptions(sensor) {
    let opts = {
      sensor: sensor.getCameraInfo(this.wrapper.camera_info.camera_info_name),
      options: [],
    };
    for (let opt in this.wrapper.option) {
      if (typeof this.wrapper.option[opt] === 'string') {
        if (sensor.supportsOption(this.wrapper.option[opt])) {
          console.log(`getoption: ${opt}`);
          let obj = {
            option: this.wrapper.option[opt],
            value: sensor.getOption(this.wrapper.option[opt]),
            range: sensor.getOptionRange(this.wrapper.option[opt]),
          };
          opts.options.push(obj);
        }
      }
    }
    return opts;
  }
  // find an array of streamProfiles that matches the input streamArray data.
  _findMatchingProfiles(sensorName, streamArray) {
    let sensor = this._findSensorByName(sensorName);
    let profiles = sensor.getStreamProfiles();
    let results = [];

    console.log(streamArray);
    streamArray.forEach((s) => {
      profiles.forEach((p) => {
        if (p instanceof this.wrapper.VideoStreamProfile) {
          if (this.wrapper.stream.streamToString(p.streamValue) === s.stream &&
              this.wrapper.format.formatToString(p.format) === s.format &&
              p.fps == s.fps &&
              `${p.width}*${p.height}` === s.resolution &&
              (s.index === undefined || p.streamIndex == s.index)) {
            results.push(p);
          }
        }
      });
    });
    return results;
  }

  _findSensorByName(sensorName) {
    for (let sensor of this.sensors) {
      if (sensor.getCameraInfo(this.wrapper.camera_info.camera_info_name) === sensorName) {
        return sensor;
      }
    }
    return undefined;
  }
  _processFrameBeforeSend(sensor, frame) {
    const streamType = frame.streamType;
    const width = frame.width;
    const height = frame.height;
    const streamIndex = frame.profile.streamIndex;
    const format = frame.format;

    return new Promise((resolve, reject) => {
      if (streamType === this.wrapper.stream.STREAM_COLOR) {
        sharp(Buffer.from(frame.data.buffer), {
          raw: {
            width: width,
            height: height,
            channels: 3,
          },
        }).jpeg({
          quality: jpegQuality,
        }).toBuffer().then((data) => {
          let result = {
            meta: {
              stream: this.wrapper.stream.streamToString(streamType),
              index: frame.profile.streamIndex,
              format: this.wrapper.format.formatToString(format),
              width: width,
              height: height,
            },
            data: data,
          };
          resolve(result);
        });
      } else if (streamType === this.wrapper.stream.STREAM_DEPTH) {
        const depthMap = this.colorizer.colorize(frame);
        sharp(Buffer.from(depthMap.data.buffer), {
          raw: {
            width: width,
            height: height,
            channels: 3,
          },
        }).jpeg({
          quality: jpegQuality,
        }).toBuffer().then((data) => {
          let result = {
            meta: {
              stream: this.wrapper.stream.streamToString(streamType),
              index: frame.profile.streamIndex,
              format: this.wrapper.format.formatToString(format),
              width: width,
              height: height,
            },
            data: data,
          };
          resolve(result);
        });
      } else if (streamType === this.wrapper.stream.STREAM_INFRARED) {
        const infraredFrame = this.decimate.process(frame);
        // const infraredFrame = frame;
        let result = {
          meta: {
            stream: this.wrapper.stream.streamToString(streamType)+streamIndex,
            index: frame.profile.streamIndex,
            format: this.wrapper.format.formatToString(format),
            width: infraredFrame.width,
            height: infraredFrame.height,
          },
          data: infraredFrame.data,
          frame: infraredFrame,
        };
        resolve(result);
      }
    });
  }
  // process the start command
  _handleStart(cmd) {
    let profiles = this._findMatchingProfiles(cmd.data.sensor, cmd.data.streams);
    let sensor = this._findSensorByName(cmd.data.sensor);
    if (profiles && sensor) {
      console.log('open profiles:');
      console.log(profiles);
      sensor.open(profiles);
      sensor.start((frame) => {
        this._processFrameBeforeSend(sensor, frame).then((output) => {
          connectMgr.sendProcessedFrameData(output);
          this.sendCount[output.meta.stream]++;
        });
      });
    }
  }
  // process set option command
  _handleSetOption(cmd) {
    let sensor = this._findSensorByName(cmd.data.sensor);
    sensor.setOption(cmd.data.option, Number(cmd.data.value));
  }
}

class ConnectionManager {
  constructor(port, wsPort) {
    this.app = express();
    this.cmdSocket = null;
    this.dataSockets = new Map();
    this.dataSocketsReady = new Map();
    this.wsPort = wsPort;
    this.port = port;
  }
  connectionCheck(req, res, next) {
    if (connectMgr.cmdSocket) {
      res.send('A connection already exist!');
    } else {
      next();
    }
  }
  start() {
    this.app.all('*', this.connectionCheck);
    this.app.use(express.static(path.join(__dirname, 'public')));
    this.wss = new WebSocket.Server({port: this.wsPort});

    this.wss.on('connection', (socket) => {
      if (!this.cmdSocket) {
        this.acceptCommandConnection(socket);
      } else {
        socket.addEventListener('message', (event) => {
          let streamObj = JSON.parse(event.data);
          this.dataSockets[streamObj.stream] = socket;
          this.dataSocketsReady[streamObj.stream] = true;
          console.log(
              `${streamObj.stream} connection established! ${this.dataSockets[streamObj.stream]}`);
        });
      }
    });
    this.app.listen(port);
    console.log(`listening on http://localhost:${this.port}`);
  }
  acceptCommandConnection(socket) {
    console.log('Command connection established!');
    this.cmdSocket = socket;
    this.cmdSocket.on('close', () => {
      rsObj.stop();
      rsObj.cleanup();
      this.cmdSocket = null;
    });
    this.cmdSocket.on('message', (msg) => {
      rsObj.processCommand(JSON.parse(msg));
    });
    if (rsObj.isReady) {
      this.sendCmdObject(rsObj.getAllSensorInfo());
      this.sendCmdObject(rsObj.getPresets());
      this.sendCmdObject(rsObj.getOptions());
      this.sendCmdObject(rsObj.getDefaultConfig());
    } else {
      this.sendCmdObject({tag: ResponseTag.error, description: 'No camera found!'});
    }
  }
  sendProcessedFrameData(data) {
    this.sendData(data.meta.stream, data.meta, true);
    this.sendData(data.meta.stream, data.data);
  }
  sendCmdObject(obj) {
    this.cmdSocket.send(JSON.stringify(obj));
  }
  sendData(streamName, data, isString = false) {
    if (!this.dataSocketsReady[streamName]) {
      console.error(`${streamName} data connection not established!`);
    }
    if (isString) {
      this.dataSockets[streamName].send(JSON.stringify(data));
    } else {
      this.dataSockets[streamName].send(data);
    }
  }
}
let connectMgr = new ConnectionManager(port, wsPort);
let rsObj = new Realsense(rsWrapper, connectMgr);
rsObj.init();
connectMgr.start();
