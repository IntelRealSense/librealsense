// Copyright (c) 2017 Intel Corporation. All rights reserved.
// Use of this source code is governed by an Apache 2.0 license
// that can be found in the LICENSE file.

'use strict';

/* global describe, it, before, after, afterEach */
const assert = require('assert');
const fs = require('fs');
let rs2;
try {
  rs2 = require('node-librealsense');
} catch (e) {
  rs2 = require('../index.js');
}

let isRecord;
let isPlayback;
let recordFile;

for (let i = 0; i < process.argv.length; i++) {
  if (process.argv[i] === '--record') {
    isRecord = true;
    recordFile = process.argv[i + 1];
  } else if (process.argv[i] === '--playback') {
    isPlayback = true;
    recordFile = process.argv[i + 1];
  }
}

if (isRecord || isPlayback) {
  if (!recordFile || typeof recordFile !== 'string') {
    throw new TypeError('record/playback file must be provided like\'--playback ut_record.data\'');
  }
  if (isPlayback && !fs.existsSync(recordFile)) {
    throw new TypeError('the playback file \'' + recordFile + '\' doesn\'t exists');
  }
}

function makeContext(section) {
  if (isRecord) {
    return new rs2.internal.RecordingContext(recordFile, section);
  } else if (isPlayback) {
    return new rs2.internal.PlaybackContext(recordFile, section);
  } else {
    return new rs2.Context();
  }
}

describe('Pipeline tests', function() {
  let ctx;
  let pipe;
  this.timeout(5000);

  before(function() {
    ctx = makeContext('pipeline');
    pipe = new rs2.Pipeline(ctx);
    const devices = ctx.queryDevices().devices;
    assert(devices.length > 0); // Device must be connected
  });

  after(function() {
    rs2.cleanup();
  });

  if (!(isRecord || isPlayback)) {
    it('Default pipeline', () => {
      const pipe = new rs2.Pipeline();
      pipe.start();
      const frames = pipe.waitForFrames();
      assert.equal(frames.size > 0, true);
      pipe.stop();
    });
  }

  it('Pipeline with context', () => {
    pipe.start();
    const frames = pipe.waitForFrames();
    assert.equal(frames.size > 0, true);
    pipe.stop();
  });

  it('pipeline pollForFrames', () => {
    pipe.start();
    let frames;
    while (!frames) {
      frames = pipe.pollForFrames();
      if (frames) {
        assert.equal((frames instanceof rs2.FrameSet), true);
      }
    }
    pipe.stop();
  });
});

describe('Frameset test', function() {
  let pipe;
  let frameset;
  let ctx;
  this.timeout(5000);

  before(function() {
    ctx = makeContext('frameset');
    pipe = new rs2.Pipeline(ctx);
    pipe.start();
    frameset = pipe.waitForFrames();
  });

  after(function() {
    pipe.stop();
    rs2.cleanup();
  });

  it('depthFrame test', () => {
    let depth = frameset.depthFrame;
    assert.equal(depth instanceof rs2.DepthFrame, true);
    let distance = depth.getDistance(100, 100);
    assert.equal(typeof distance, 'number');
  });

  it('colorFrame test', () => {
    let color = frameset.colorFrame;
    assert.equal(color instanceof rs2.VideoFrame, true);
  });

  it('at test', () => {
    for (let i=0; i<frameset.size; i++) {
      let frame = frameset.at(i);
      assert.equal(frame instanceof rs2.Frame, true);
    }
  });

  it('getFrame test', () => {
    let color = frameset.getFrame(rs2.stream.STREAM_COLOR);
    let depth = frameset.getFrame(rs2.stream.STREAM_DEPTH);
    assert.equal(color instanceof rs2.VideoFrame, true);
    assert.equal(depth instanceof rs2.DepthFrame, true);
  });

  it('forEach test', () => {
    let counter = 0;
    function callback(frame) {
      counter++;
      assert.equal(frame instanceof rs2.Frame, true);
    }
    frameset.forEach(callback);
    assert.equal(counter, frameset.size);
  });
});

describe('Frame test', function() {
  let pipe;
  let frameset;
  let color;
  let depth;
  let ctx;
  this.timeout(5000);

  before(function() {
    ctx = makeContext('frame');
    pipe = new rs2.Pipeline(ctx);
    pipe.start();
    frameset = pipe.waitForFrames();
    color = frameset.colorFrame;
    depth = frameset.depthFrame;
  });

  after(function() {
    pipe.stop();
    rs2.cleanup();
  });

  it('format/stream/width/height/frameNumber/timestamp/isValid test', () => {
    assert.equal(depth.format, rs2.format.FORMAT_Z16);
    assert.equal(depth.streamType, rs2.stream.STREAM_DEPTH);
    assert.equal(depth.isValid, true);
    assert.equal(depth.timestamp > 0, true);
    assert.equal(depth.frameNumber > 0, true);
    assert.equal(depth.width > 0, true);
    assert.equal(depth.height > 0, true);
    assert.equal(depth.dataByteLength > 0, true);
    assert.equal(depth.strideInBytes > 0, true);

    assert.equal(color.format, rs2.format.FORMAT_RGB8);
    assert.equal(color.streamType, rs2.stream.STREAM_COLOR);
    assert.equal(color.isValid, true);
    assert.equal(color.timestamp > 0, true);
    assert.equal(color.frameNumber > 0, true);
    assert.equal(color.width > 0, true);
    assert.equal(color.height > 0, true);
    assert.equal(color.dataByteLength > 0, true);
    assert.equal(color.strideInBytes > 0, true);
  });

  it('frame metadata test', () => {
    for (let i=0; i<rs2.frame_metadata.FRAME_METADATA_COUNT; i++) {
      if (depth.supportsFrameMetadata(i)) {
        assert.equal(depth.frameMetadata(i) != undefined, true);
      }
      if (color.supportsFrameMetadata(i)) {
        assert.equal(color.frameMetadata(i) != undefined, true);
      }
    }
  });

  it('frame data test', () => {
    assert.equal(depth.data.length*2, depth.dataByteLength);
    assert.equal(color.data.length, color.dataByteLength);
  });

  it('strideInBytes test', () => {
    assert.equal(depth.strideInBytes, depth.width*2);
    assert.equal(color.strideInBytes, color.width*3);
  });

  it('getData test', () => {
    const buf1 = new ArrayBuffer(depth.dataByteLength);
    depth.getData(buf1);
    const buf2 = Buffer.from(buf1);
    const buf3 = Buffer.from(depth.data.buffer);
    assert.equal(buf3.equals(buf2), true);
    const buf21 = new ArrayBuffer(color.dataByteLength);
    color.getData(buf21);
    const buf22 = Buffer.from(buf21);
    const buf23 = Buffer.from(color.data.buffer);
    assert.equal(buf23.equals(buf22), true);
  });
});

if (!(isRecord || isPlayback)) {
  
  describe('Colorizer test', function() {
    let pipe;
    let frameset;
    let depth;
    let colorizer;
    let ctx;
    this.timeout(5000);

    before(function() {
      ctx = makeContext('colorizer');
      pipe = new rs2.Pipeline(ctx);
      pipe.start();
      frameset = pipe.waitForFrames();
      depth = frameset.depthFrame;
      colorizer = new rs2.Colorizer();
    });

    after(function() {
      pipe.stop();
      rs2.cleanup();
    });

    it('colorize test', () => {
      const depthRGB = colorizer.colorize(depth);
      assert.equal(depthRGB.height, depth.height);
      assert.equal(depthRGB.width, depth.width);
      assert.equal(depthRGB.format, rs2.format.FORMAT_RGB8);
    });
    it('colorizer option API test', () => {
      for (let i = rs2.option.OPTION_BACKLIGHT_COMPENSATION; i < rs2.option.OPTION_COUNT; i++) {
        let readonly = colorizer.isOptionReadOnly(i);
        assert.equal((readonly === undefined) || (typeof readonly === 'boolean'), true);
        let supports = colorizer.supportsOption(i);
        assert.equal((supports === undefined) || (typeof supports === 'boolean'), true);
        let value = colorizer.getOption(i);
        assert.equal((value === undefined) || (typeof value === 'number'), true);
        let des = colorizer.getOptionDescription(i);
        assert.equal((des === undefined) || (typeof des === 'string'), true);
        des = colorizer.getOptionValueDescription(i, 1);
        assert.equal((des === undefined) || (typeof des === 'string'), true);

        if (supports && !readonly) {
          let range = colorizer.getOptionRange(i);
          for (let j = range.minvalue; j <= range.maxValue; j += range.step) {
            colorizer.setOption(i, j);
            let val = colorizer.getOption(i);
            assert.equal(val, j);
          }
        }
      }
    });
  });

  describe('Pointcloud and Points test', function() {
    let pipe;
    let frameset;
    let color;
    let depth;
    let pc;
    let ctx;

    before(function() {
      ctx = new rs2.Context();
      pc = new rs2.PointCloud();
      pipe = new rs2.Pipeline(ctx);
      pipe.start();
      frameset = pipe.waitForFrames();
      color = frameset.colorFrame;
      depth = frameset.depthFrame;
    });

    after(function() {
      pipe.stop();
      rs2.cleanup();
    });

    it('map and calculate test', () => {
      assert.equal(pc instanceof rs2.PointCloud, true);

      pc.mapTo(color);
      const points = pc.calculate(depth);
      const cnt = depth.width * depth.height;
      assert.equal(points instanceof rs2.Points, true);
      assert.equal(points.size, cnt);
      const texCoordinates = points.textureCoordinates;

      assert.equal(points.vertices instanceof Float32Array, true);
      assert.equal(texCoordinates instanceof Int32Array, true);

      assert.equal(points.vertices.length, cnt * 3);
      assert.equal(texCoordinates.length, cnt * 2);
    });
  });
}

describe('Context tests', function() {
  let ctx;
  this.timeout(5000);

  before(() => {
    ctx = makeContext('context');
  });

  after(() => {
    rs2.cleanup();
  });

  it('Query devices', () => {
    const devList = ctx.queryDevices();
    assert.equal(devList instanceof rs2.DeviceList, true);
    // The platform's camera would also be counted, so size may > 1
    assert.equal(devList.size >= 1, true);
    assert.equal(devList.devices[0] instanceof rs2.Device, true);
    assert.equal(devList.contains(devList.devices[0]), true);
    devList.destroy();
  });

  it('Query sensors', () => {
    const sensors = ctx.querySensors();
    // The platform's camera would also be counted, so size may > 2
    assert.equal(sensors.length >= 2, true);
    assert.equal(sensors[0] instanceof rs2.Sensor, true);
    assert.equal(sensors[1] instanceof rs2.Sensor, true);
    sensors.forEach((sensor) => {
      sensor.destroy();
    });
  });

  it('Get sensor parent', () => {
    const sensors = ctx.querySensors();
    const dev = ctx.getSensorParent(sensors[0]);
    assert.equal(dev instanceof rs2.Device, true);
    sensors.forEach((sensor) => {
      sensor.destroy();
    });
  });
});

describe('Sensor tests', function() {
  let ctx;
  let sensors;
  this.timeout(10000);

  before(() => {
    ctx = makeContext('sensor');
    sensors = ctx.querySensors();
  });

  after(() => {
    rs2.cleanup();
  });

  it('camera info', () => {
    sensors.forEach((sensor) => {
      for (let i = rs2.camera_info.CAMERA_INFO_NAME; i < rs2.camera_info.CAMERA_INFO_COUNT; i++) {
        if (sensor.supportsCameraInfo(i)) {
          let info = sensor.getCameraInfo(i);
          assert.equal(typeof info, 'string');
        }
      }
    });
  });

  it('Stream profiles', () => {
    const profiles0 = sensors[0].getStreamProfiles();
    const profiles1 = sensors[1].getStreamProfiles();

    assert.equal(profiles1.length > 0, true);
    assert.equal(profiles0.length > 0, true);
    profiles0.forEach((p) => {
      assert.equal(p instanceof rs2.StreamProfile, true);
      assert.equal(p instanceof rs2.StreamProfile, true);
      assert.equal(p.streamType >= rs2.stream.STREAM_DEPTH &&
                   p.streamType < rs2.stream.STREAM_COUNT, true);
      assert.equal(p.format >= rs2.format.FORMAT_Z16 && p.format < rs2.format.FORMAT_COUNT, true);
      assert.equal(p.fps>0, true);
      assert.equal(typeof p.uniqueID, 'number');
      assert.equal(typeof p.isDefault, 'boolean');
    });
    profiles1.forEach((p) => {
      assert.equal(p instanceof rs2.StreamProfile, true);
      assert.equal(p instanceof rs2.StreamProfile, true);
    });
    const extrin = profiles0[0].getExtrinsicsTo(profiles1[0]);
    assert.equal(('rotation' in extrin), true);
    assert.equal(('translation' in extrin), true);

    profiles0.forEach((p) => {
      if (p instanceof rs2.VideoStreamProfile) {
        assert.equal(typeof p.width, 'number');
        assert.equal(typeof p.height, 'number');
        let intrin;
        try {
          intrin = p.getIntrinsics();
        } catch (e) {
          // some stream profiles have no intrinsics, and will trigger exception
          // so only bypass these cases if it's still recoverable, otherwise,
          // rethrow it.
          if (e instanceof rs2.UnrecoverableError) {
            throw e;
          }
          return;
        }

        assert.equal('width' in intrin, true);
        assert.equal('height' in intrin, true);
        assert.equal('ppx' in intrin, true);
        assert.equal('ppy' in intrin, true);
        assert.equal('fx' in intrin, true);
        assert.equal('fy' in intrin, true);
        assert.equal('model' in intrin, true);
        assert.equal('coeffs' in intrin, true);

        assert.equal(typeof intrin.width, 'number');
        assert.equal(typeof intrin.height, 'number');
        assert.equal(typeof intrin.ppx, 'number');
        assert.equal(typeof intrin.ppy, 'number');
        assert.equal(typeof intrin.fx, 'number');
        assert.equal(typeof intrin.fy, 'number');
        assert.equal(typeof intrin.model, 'number');
        assert.equal(Array.isArray(intrin.coeffs), true);

        const fov = rs2.util.fov(intrin);
        assert.equal(typeof fov.h, 'number');
        assert.equal(typeof fov.v, 'number');
      }
    });
  });

  it('Open and start', () => {
    return new Promise((resolve, reject) => {
      const profiles0 = sensors[0].getStreamProfiles();
      sensors[0].open(profiles0[0]);
      let started = true;
      sensors[0].start((frame) => {
        assert.equal(frame instanceof rs2.Frame, true);
        // Add a timeout to stop to avoid failure during playback test
        // and make sure to stop+close sensors[0] once
        if (started) {
          started = false;
          setTimeout(() => {
            sensors[0].stop();
            sensors[0].close();
            resolve();
          }, 0);
        }
      });
    });
  });
  it('Get depth scale', () => {
    for (let i = 0; i < sensors.length; i++) {
      if (sensors[i] instanceof rs2.DepthSensor) {
        assert.equal(typeof sensors[i].depthScale === 'number', true);
      }
    }
  });
  it('getOptionDescription', () => {
    sensors.forEach((s) => {
      for (let i = rs2.option.OPTION_BACKLIGHT_COMPENSATION; i < rs2.option.OPTION_COUNT; i++) {
        let des = s.getOptionDescription(i);
        assert.equal((des === undefined) || (typeof des === 'string'), true);
      }
    });
  });
  it('getOption', () => {
    sensors.forEach((s) => {
      for (let i = rs2.option.OPTION_BACKLIGHT_COMPENSATION; i < rs2.option.OPTION_COUNT; i++) {
        let value = s.getOption(i);
        assert.equal((value === undefined) || (typeof value === 'number'), true);
      }
    });
  });
  it('getOptionValueDescription', () => {
    sensors.forEach((s) => {
      for (let i = rs2.option.OPTION_BACKLIGHT_COMPENSATION; i < rs2.option.OPTION_COUNT; i++) {
        let des = s.getOptionValueDescription(i, 1);
        assert.equal((des === undefined) || (typeof des === 'string'), true);
      }
    });
  });
  it('setOption', () => {
    sensors.forEach((s) => {
      for (let i = rs2.option.OPTION_BACKLIGHT_COMPENSATION; i < rs2.option.OPTION_COUNT; i++) {
        if (s.supportsOption(i) && !s.isOptionReadOnly(i)) {
          let range = s.getOptionRange(i);
          for (let j = range.minvalue; j <= range.maxValue; j += range.step) {
            s.setOption(i, j);
            let val = s.getOption(i);
            assert.equal(val, j);
          }
        }
      }
    });
  });
  it('Notification test', () => {
    return new Promise((resolve, reject) => {
      let dev = ctx.queryDevices().devices[0];
      sensors[0].setNotificationsCallback((n) => {
        assert.equal(typeof n.descr, 'string');
        assert.equal(typeof n.timestamp, 'number');
        assert.equal(typeof n.severity, 'string');
        assert.equal(typeof n.category, 'string');
        assert.equal(typeof n.serializedData, 'string');
        resolve();
      });
      // No need to manually trigger error during playback.
      if (!isPlayback) {
        setTimeout(() => {
          dev.cxxDev.triggerErrorForTest();
        }, 5000);
      }
    });
  });
});

// describe('Align tests', function() {
//  let ctx;
//  let align;
//  let pipe;

//  before(() => {
//    ctx = makeContext('align');
//    align = new rs2.Align(rs2.stream.STREAM_COLOR);
//    pipe = new rs2.Pipeline(ctx);
//  });

//  after(() => {
//    pipe.stop();
//    rs2.cleanup();
//  });

//  it('process', () => {
//    pipe.start();
//    const frameset = pipe.waitForFrames();
//    const output = align.process(frameset);
//    const color = output.colorFrame;
//    const depth = output.depthFrame;
//    assert.equal(color instanceof rs2.VideoFrame, true);
//    assert.equal(depth instanceof rs2.DepthFrame, true);
//  });
// });

describe(('syncer test'), function() {
  let syncer;
  let ctx;
  let sensors;
  this.timeout(5000);

  before(() => {
    ctx = makeContext('syncer');
    syncer = new rs2.Syncer();
    sensors = ctx.querySensors();
  });

  after(() => {
    sensors[0].stop();
    rs2.cleanup();
  });
  it('sensor.start(syncer)', () => {
    const profiles = sensors[0].getStreamProfiles();
    sensors[0].open(profiles[0]);
    sensors[0].start(syncer);

    let frames = syncer.waitForFrames(5000);
    assert.equal(frames instanceof rs2.FrameSet, true);
    let gotFrame = false;
    while (!gotFrame) {
      let frames = syncer.pollForFrames();
      if (frames) {
        assert.equal(frames instanceof rs2.FrameSet, true);
        gotFrame = true;
      }
    }
    profiles.forEach((p) => {
      p.destroy();
    });
  });
});

describe('Config test', function() {
  let pipe;
  let cfg;
  let ctx;
  this.timeout(5000);

  before(function() {
    ctx = makeContext('config');
    pipe = new rs2.Pipeline(ctx);
    cfg = new rs2.Config();
  });

  after(function() {
    pipe.stop();
    rs2.cleanup();
  });

  it('resolve test', function() {
    assert.equal(cfg.canResolve(pipe), true);
    const profile = cfg.resolve(pipe);
    assert.equal(profile instanceof rs2.PipelineProfile, true);
    assert.equal(profile.isValid, true);
    const dev = profile.getDevice();
    assert.equal(dev instanceof rs2.Device, true);
    const profiles = profile.getStreams();
    assert.equal(Array.isArray(profiles), true);
    assert.equal(profiles[0] instanceof rs2.StreamProfile, true);
    const colorProfile = profile.getStream(rs2.stream.STREAM_COLOR);
    assert.equal(colorProfile.streamType, rs2.stream.STREAM_COLOR);
  });

  it('enableStream test', function() {
    cfg.disableAllStreams();
    cfg.enableStream(rs2.stream.STREAM_COLOR, -1, 640, 480, rs2.format.FORMAT_RGB8, 30);
    const profile = cfg.resolve(pipe);
    assert.equal(profile instanceof rs2.PipelineProfile, true);
    const profiles = profile.getStreams();
    assert.equal(Array.isArray(profiles), true);
    assert.equal(profiles.length, 1);
    assert.equal((profiles[0] instanceof rs2.VideoStreamProfile), true);
    assert.equal(profiles[0].streamType, rs2.stream.STREAM_COLOR);
    assert.equal(profiles[0].format, rs2.format.FORMAT_RGB8);
    assert.equal(profiles[0].width, 640);
    assert.equal(profiles[0].height, 480);
    assert.equal(profiles[0].fps, 30);

    const startProfile = pipe.start(cfg);
    const startProfiles = startProfile.getStreams();
    assert.equal(Array.isArray(startProfiles), true);
    assert.equal(startProfiles.length, 1);
    assert.equal((startProfiles[0] instanceof rs2.VideoStreamProfile), true);
    assert.equal(startProfiles[0].streamType, rs2.stream.STREAM_COLOR);
    assert.equal(startProfiles[0].format, rs2.format.FORMAT_RGB8);
    assert.equal(startProfiles[0].width, 640);
    assert.equal(startProfiles[0].height, 480);
    assert.equal(startProfiles[0].fps, 30);

    const activeProfile = pipe.getActiveProfile();
    const activeProfiles = activeProfile.getStreams();
    assert.equal(Array.isArray(activeProfiles), true);
    assert.equal(activeProfiles.length, 1);
    assert.equal((activeProfiles[0] instanceof rs2.VideoStreamProfile), true);
    assert.equal(activeProfiles[0].streamType, rs2.stream.STREAM_COLOR);
    assert.equal(activeProfiles[0].format, rs2.format.FORMAT_RGB8);
    assert.equal(activeProfiles[0].width, 640);
    assert.equal(activeProfiles[0].height, 480);
    assert.equal(activeProfiles[0].fps, 30);
  });
});

describe(('DeviceHub test'), function() {
  let hub;
  let ctx;

  before(() => {
    ctx = makeContext('deviceHub');
    hub = new rs2.DeviceHub(ctx);
  });

  after(() => {
    rs2.cleanup();
  });
  it('API test', () => {
    const dev = hub.waitForDevice();
    assert.equal(dev instanceof rs2.Device, true);
    assert.equal(hub.isConnected(dev), true);
    dev.destroy();
  });
});

if (!(isRecord || isPlayback)) {
  describe(('record & playback test'), function() {
    let fileName = 'ut-record.bag';

    afterEach(() => {
      rs2.cleanup();
    });

    after(() => {
      if (fs.existsSync(fileName)) {
        fs.unlinkSync(fileName);
      }
    });

    function startRecording(file, cnt, callback) {
      return new Promise((resolve, reject) => {
        setTimeout(() => {
          let ctx = new rs2.Context();
          let dev = ctx.queryDevices().devices[0];
          let recorder = new rs2.RecorderDevice(file, dev);
          let sensors = recorder.querySensors();
          let sensor = sensors[0];
          let profiles = sensor.getStreamProfiles();
          assert.equal(recorder.fileName, file);
          for (let i = 0; i < profiles.length; i++) {
            if (profiles[i].streamType === rs2.stream.STREAM_DEPTH &&
                profiles[i].fps === 30 &&
                profiles[i].width === 640 &&
                profiles[i].height === 480 &&
                profiles[i].format === rs2.format.FORMAT_Z16) {
              sensor.open(profiles[i]);
            }
          }
          let counter = 0;
          sensor.start((frame) => {
            if (callback) {
              callback(recorder, counter);
            }
            counter++;
            if (counter === cnt) {
              recorder.reset();
              rs2.cleanup();
              resolve();
            }
          });
        }, 2000);
      });
    }

    function startPlayback(file, callback) {
      return new Promise((resolve, reject) => {
        let ctx = new rs2.Context();
        let dev = ctx.loadDevice(file);
        let sensors = dev.querySensors();
        let sensor = sensors[0];
        let profiles = sensor.getStreamProfiles();
        let cnt = 0;

        dev.setStatusChangedCallback((status) => {
          callback(dev, status, cnt);
          if (status.description === 'stopped') {
            dev.stop();
            ctx.unloadDevice(file);
            resolve();
          }
        });
        sensor.open(profiles);
        sensor.start((frame) => {
          cnt++;
        });
      });
    }

    it('record test', () => {
      return new Promise((resolve, reject) => {
        startRecording(fileName, 1, null).then(() => {
          assert.equal(fs.existsSync(fileName), true);
          fs.unlinkSync(fileName);
          resolve();
        });
      });
    }).timeout(5000);

    it('pause/resume test', () => {
      return new Promise((resolve, reject) => {
        startRecording(fileName, 2, (recorder, cnt) => {
          if (cnt === 1) {
            recorder.pause();
            recorder.resume();
          }
        }).then(() => {
          assert.equal(fs.existsSync(fileName), true);
          fs.unlinkSync(fileName);
          resolve();
        });
      });
    }).timeout(5000);

    it('playback test', () => {
      return new Promise((resolve, reject) => {
        startRecording(fileName, 1, null).then(() => {
          assert.equal(fs.existsSync(fileName), true);
          return startPlayback(fileName, (playbackDev, status) => {
            if (status.description === 'playing') {
              assert.equal(playbackDev.fileName, 'ut-record.bag');
              assert.equal(typeof playbackDev.duration, 'number');
              assert.equal(typeof playbackDev.position, 'number');
              assert.equal(typeof playbackDev.isRealTime, 'boolean');
              assert.equal(playbackDev.currentStatus.description, 'playing');
            }
          });
        }).then(() => {
          resolve();
        });
      });
    }).timeout(5000);
  });
}

describe('Device tests', function() {
  let dev;
  this.timeout(5000);

  before(function() {
    let ctx = makeContext('device');
    dev = ctx.queryDevices().devices[0];
    assert(dev instanceof rs2.Device, true);
  });

  after(function() {
    rs2.cleanup();
  });

  it('first', () => {
    let sensor = dev.first;
    assert.equal(sensor instanceof rs2.Sensor, true);
  });

  it('querySensors', () => {
    let sensors = dev.querySensors();
    assert.equal(sensors.length > 0, true);
    sensors.forEach((sensor) => {
      assert.equal(sensor instanceof rs2.Sensor, true);
    });
  });

  it('camera info', () => {
    for (let i = rs2.camera_info.CAMERA_INFO_NAME; i < rs2.camera_info.CAMERA_INFO_COUNT; i++) {
      if (dev.supportsCameraInfo(i)) {
        let info = dev.getCameraInfo(i);
        assert.equal(typeof info, 'string');
      }
    }
    let obj = dev.getCameraInfo();
    assert.equal(typeof obj, 'object');
    assert.equal(typeof obj.serialNumber, 'string');
    assert.equal(typeof obj.firmwareVersion, 'string');
    assert.equal(typeof obj.physicalPort, 'string');
    assert.equal(typeof obj.debugOpCode, 'string');
    assert.equal(typeof obj.advancedMode, 'string');
    assert.equal(typeof obj.productId, 'string');
  });
});

describe('filter tests', function() {
  let ctx;
  let decimationFilter;
  let temporalFilter;
  let spatialFilter;
  let pipeline;
  this.timeout(10000);

  before(function() {
    ctx = makeContext('filter');
    decimationFilter = new rs2.DecimationFilter();
    temporalFilter = new rs2.TemporalFilter();
    spatialFilter = new rs2.SpatialFilter();
    pipeline = new rs2.Pipeline(ctx);
    pipeline.start();
  });

  after(function() {
    pipeline.stop();
    rs2.cleanup();
  });

  it('temppral filter', () => {
    let frameset = pipeline.waitForFrames();
    let depthFrame = frameset.depthFrame;
    let out = temporalFilter.process(depthFrame);
    assert.equal(out instanceof rs2.DepthFrame, true);
    assert.equal(typeof out.width, 'number');
  })

  it('spatial filter', () => {
    let frameset = pipeline.waitForFrames();
    let depthFrame = frameset.depthFrame;
    let out = spatialFilter.process(depthFrame);
    assert.equal(out instanceof rs2.DepthFrame, true);
    assert.equal(typeof out.width, 'number');
  }).timeout(10000); // change the threshold to be 10 seconds to avoid timeout.

  it('decimation filter', () => {
    let frameset = pipeline.waitForFrames();
    let depthFrame = frameset.depthFrame;
    let out = decimationFilter.process(depthFrame);
    assert.equal(out instanceof rs2.VideoFrame, true);
    assert.equal(typeof out.width, 'number');
  }).timeout(10000); // change the threshold to be 10 seconds to avoid timeout.
});
