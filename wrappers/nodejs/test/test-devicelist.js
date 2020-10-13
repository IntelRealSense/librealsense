// Copyright (c) 2018 Intel Corporation. All rights reserved.
// Use of this source code is governed by an Apache 2.0 license
// that can be found in the LICENSE file.

'use strict';

/* global describe, it, beforeEach, afterEach */
const assert = require('assert');
let rs2;
try {
  rs2 = require('node-librealsense');
} catch (e) {
  rs2 = require('../index.js');
}

let ctx;
let devl;
describe('DeviceList test', function() {
  beforeEach(function() {
    ctx = new rs2.Context();
    devl = ctx.queryDevices();
  });

  afterEach(function() {
    rs2.cleanup();
  });

  it('Testing constructor', () => {
    assert.doesNotThrow(() => {
      new rs2.DeviceList(devl);
    });
  });

  it('Testing member devices', () => {
    if (devl.size === 0 || devl.size === undefined) {
      assert.equal(devl.devices, undefined);
    }
    let output = devl.devices;
    for (let i=0; i < devl.size; i++) {
      assert(output[i] instanceof rs2.Device);
    }
  });

  it('Testing member size', () => {
    assert.equal(typeof devl.size, 'number');
  });

  it('Testing member back', () => {
    let dev = devl.devices;
    assert(dev.length > 0); // Device must be connected
    let device = devl.getDevice(dev.length -1);
    let SN = device.getCameraInfo().serialNumber;
    assert.doesNotThrow(() => {
      if (devl.size > 0) {
        let res = devl.back;
        assert(devl.back instanceof rs2.Device);
        assert.equal(typeof res, 'object');
        assert.equal(res.getCameraInfo().serialNumber, SN);
      }
    });
  });

  it('Testing member front', () => {
    let dev = devl.devices;
    assert(dev.length > 0); // Device must be connected
    let device = devl.getDevice(0);
    let SN = device.getCameraInfo().serialNumber;
    assert.doesNotThrow(() => {
      if (devl.size > 0) {
        let res = devl.front;
        assert(devl.front instanceof rs2.Device);
        assert.equal(typeof res, 'object');
        assert.equal(res.getCameraInfo().serialNumber, SN);
      }
    });
  });

  it('Testing method destroy', () => {
    assert.notEqual(devl.cxxList, undefined);
    assert.doesNotThrow(() => {
      devl.destroy();
    });
    assert.equal(devl.cxxList, undefined);
  });

  it('Testing method getDevice - without argument', () => {
    assert.throws(() => {
      devl.getDevice();
    });
  });

  it('Testing method getDevice - return value', () => {
    for (let i = 0; i < devl.size; i++) {
      let dev = devl.getDevice(i);
      if (dev) {
        assert(dev instanceof rs2.Device);
      } else {
        assert.equal(dev, undefined);
      }
    }
  });

  it('Testing method getDevice - with argument', () => {
    for (let i = 0; i < devl.size; i++) {
      assert.doesNotThrow(() => { // jshint ignore:line
        devl.getDevice(i);
      });
    }
  });

  it('Testing method getDevice - with invalid argument', () => {
    assert.throws(() => {
      devl.getDevice('dummy');
    });
  });

  it('Testing method contains - without argument', () => {
    assert.throws(() => {
      devl.contains();
    });
  });

  it('Testing method contains - return value', () => {
    for (let i = 0; i < devl.size; i++) {
      let dev = devl.getDevice(i);
      assert.equal(typeof devl.contains(dev), 'boolean');
    }
  });

  it('Testing method contains - with argument', () => {
    for (let i = 0; i < devl.size; i++) {
      assert.doesNotThrow(() => { // jshint ignore:line
        let dev = devl.getDevice(i);
        devl.contains(dev);
      });
    }
  });

  it('Testing method contains - with invalid argument', () => {
    assert.throws(() => {
      devl.contains('dummy');
    });
  });
});
