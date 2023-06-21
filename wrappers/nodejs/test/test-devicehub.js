// Copyright (c) 2017 Intel Corporation. All rights reserved.
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
let devh;
describe('DeviceHub test', function() {
  beforeEach(function() {
    ctx = new rs2.Context();
    devh = new rs2.DeviceHub(ctx);
  });

  afterEach(function() {
    rs2.cleanup();
  });

  it('Testing constructor', () => {
    assert.doesNotThrow(() => {
      new rs2.DeviceHub(ctx);
    });
  });

  it('Testing method destroy', () => {
    assert.notEqual(devh.cxxHub, undefined);
    assert.doesNotThrow(() => {
      devh.destroy();
    });
    assert.equal(devh.cxxHub, undefined);
  });

  it('Testing method isConnected', () => {
    let devs = ctx.queryDevices().devices;
    devs.forEach((dev) => {
      assert.doesNotThrow(() => {
        devh.isConnected(dev);
      });
      assert(devh.isConnected(dev));
    });
  });

  it('Testing method isConnected - without argument', () => {
    assert.throws(() => {
      devh.isConnected();
    });
  });

  it('Testing method isConnected - with invalid argument', () => {
    assert.throws(() => {
      devh.isConnected('dummy');
    });
  });

  it('Testing method waitForDevice', () => {
    assert.doesNotThrow(() => {
      devh.waitForDevice();
    });
  });

  it('Testing method waitForDevice - return value', () => {
    let dev = devh.waitForDevice();
    if (dev) {
      assert(dev instanceof rs2.Device);
    } else {
      assert.equal(dev, undefined);
    }
  });

  it('Testing method waitForDevice - with invalid argument', () => {
    let dev = devh.waitForDevice('dummy');
    assert(dev instanceof rs2.Device);
  });

  it('Testing method waitForDevice - call multiple times', () => {
    let dev;
    assert.doesNotThrow(() => {
      dev = devh.waitForDevice();
      dev = devh.waitForDevice();
    });
    if (dev) {
      assert(dev instanceof rs2.Device);
    } else {
      assert.equal(dev, undefined);
    }
  });
});
