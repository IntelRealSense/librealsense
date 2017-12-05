// Copyright (c) 2017 Intel Corporation. All rights reserved.
// Use of this source code is governed by an Apache 2.0 license
// that can be found in the LICENSE file.

'use strict';

/* global describe, it, before, after */
const assert = require('assert');
const EventEmitter = require('events');
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

  // getTime is removed
  it.skip('testing method - getTime, should get number value', () => {
    const context = new librealsense2.Context();
    assert.doesNotThrow(() => {
      context.getTime();
    });
    assert.equal(typeof context.getTime(), 'number');
  });

  it.skip('testing method - getTime, should get current system time', () => {
    const context = new librealsense2.Context();
    assert.doesNotThrow(() => {
      context.getTime();
    });
    const currentTime = new Date();
    assert(context.getTime() - currentTime > 0 &&
      context.getTime() - currentTime < 5);
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
    assert.doesNotThrow(() => {
      context.setDevicesChangedCallback();
    });
  });

  // isDeviceConnected is no more a method of Context
  it.skip('testing method - isDeviceConnected', () => {
    const context = new librealsense2.Context();
    const devs = context.queryDevices().devices;
    assert(devs[0]);
    const dev = devs[0];
    assert.doesNotThrow(() => {
      context.isDeviceConnected(dev);
    });
  });

  // isDeviceConnected is no more a method of Context
  it.skip('testing method - isDeviceConnected, return value', () => {
    const context = new librealsense2.Context();
    const devs = context.queryDevices().devices;
    assert(devs[0]);
    const dev = devs[0];
    assert.equal(typeof context.isDeviceConnected(dev), 'boolean');
    assert.equal(context.isDeviceConnected(dev), true);
  });

  it('testing method - isDeviceConnected, with invalid options', () => {
    const context = new librealsense2.Context();
    let opt;
    assert.throws(() => {
      context.isDeviceConnected();
    });
    assert.throws(() => {
      context.isDeviceConnected(opt);
    });
    assert.throws(() => {
      context.isDeviceConnected(null);
    });
  });

  it('testing method - loadDevice, return playbackDevice', () => {
    const context = new librealsense2.Context();
    let pbd;
    assert.doesNotThrow(() => {
      pbd = context.loadDevice();
    });
    assert(pbd instanceof librealsense2.Device);
  });
});
