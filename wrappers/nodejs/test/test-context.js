// Copyright (c) 2017 Intel Corporation. All rights reserved.
// Use of this source code is governed by an Apache 2.0 license
// that can be found in the LICENSE file.

'use strict';

/* global describe, it, before, after */
const assert = require('assert');
const EventEmitter = require('events');
const fs = require('fs');
let librealsense2;
try {
  librealsense2 = require('node-librealsense');
} catch (e) {
  librealsense2 = require('../index.js');
}

describe('Context test', function() {
  before(function() {
    const ctx = new librealsense2.Context();
    const devices = ctx.queryDevices().devices;
    assert(devices.length > 0); // Device must be connected
  });

  after(function() {
    librealsense2.cleanup();
  });

  function startRecording(file, cnt, callback) {
    return new Promise((resolve, reject) => {
      setTimeout(() => {
        let ctx = new librealsense2.Context();
        let dev = ctx.queryDevices().devices[0];
        let recorder = new librealsense2.RecorderDevice(file, dev);
        let sensors = recorder.querySensors();
        let sensor = sensors[0];
        let profiles = sensor.getStreamProfiles();
        assert.equal(recorder.fileName, file);
        for (let i = 0; i < profiles.length; i++) {
          if (profiles[i].streamType === librealsense2.stream.STREAM_DEPTH &&
              profiles[i].fps === 30 &&
              profiles[i].width === 640 &&
              profiles[i].height === 480 &&
              profiles[i].format === librealsense2.format.FORMAT_Z16) {
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
            librealsense2.cleanup();
            resolve();
          }
        });
      }, 2000);
    });
  }

  it('testing constructor', () => {
    assert.doesNotThrow(() => {
      new librealsense2.Context();
    });
    assert.throws(() => {
      new librealsense2.Context(123);
    });
    assert.throws(() => {
      new librealsense2.Context('dummy');
    });
    const context = new librealsense2.Context();
    assert.equal(typeof context, 'object');
  });

  it('testing member - events', () => {
    const context = new librealsense2.Context();
    assert(context._events instanceof EventEmitter);
  });

  it('testing constructor - new Context, should get Context', () => {
    let context;
    assert.doesNotThrow(() => {
      context = new librealsense2.Context();
    });
    assert(context instanceof librealsense2.Context);
  });

  it('testing constructor - new Context, invalid 1 option', () => {
    assert.throws(() => {
      new librealsense2.Context(1);
    });
  });

  it('testing method - destroy, call 1 time', () => {
    assert.doesNotThrow(() => {
      const context = new librealsense2.Context();
      context.destroy();
    });
    assert.doesNotThrow(() => {
      const context = new librealsense2.Context();
      context.destroy(123);
    });
  });

  it('testing method - destroy, call 2 times', () => {
    assert.doesNotThrow(() => {
      const context = new librealsense2.Context();
      context.destroy();
      context.destroy();
    });
  });

  it('testing method - queryDevices, should get array', () => {
    const context = new librealsense2.Context();
    assert.doesNotThrow(() => {
      context.queryDevices();
    });
    const obj = context.queryDevices().devices;
    assert.equal(Object.prototype.toString.call(obj), '[object Array]');
    assert.equal(obj[0] instanceof librealsense2.Device, true);
  });

  it('testing method - queryDevices, should be instance of Device', () => {
    const context = new librealsense2.Context();
    assert.doesNotThrow(() => {
      context.queryDevices();
    });
    const obj = context.queryDevices().devices;
    assert.equal(obj[0] instanceof librealsense2.Device, true);
  });

  it('testing method - querySensors, should get array', () => {
    const context = new librealsense2.Context();
    assert.doesNotThrow(() => {
      context.querySensors();
    });
    const obj = context.querySensors();
    assert.equal(Object.prototype.toString.call(obj), '[object Array]');
    assert.equal(obj[0] instanceof librealsense2.Sensor, true);
  });

  it('testing method - querySensors, should be instance of Sensor', () => {
    const context = new librealsense2.Context();
    assert.doesNotThrow(() => {
      context.querySensors();
    });
    const obj = context.querySensors();
    assert.equal(obj[0] instanceof librealsense2.Sensor, true);
  });

  it('testing method - setDevicesChangedCallback', () => {
    const context = new librealsense2.Context();
    assert.throws(() => {
      context.setDevicesChangedCallback();
    });
  });

  it('testing method - loadDevice, return playbackDevice', () => {
    return new Promise((resolve, reject) => {
      startRecording('record.bag', 10, null).then(() => {
        const context = new librealsense2.Context();
        let pbd;
        assert.doesNotThrow(() => {
          pbd = context.loadDevice('record.bag');
        });
        assert(pbd instanceof librealsense2.Device);
        fs.unlinkSync('record.bag');
        resolve();
      });
    });
  }).timeout(5000);

  it('testing method - getSensorParent', () => {
    const context = new librealsense2.Context();
    const devices = context.queryDevices().devices;
    const sensors = devices[0].querySensors();
    context.getSensorParent(sensors[0]);
  });
});
