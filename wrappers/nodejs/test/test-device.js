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
let dev;
describe('Device test', function() {
  beforeEach(function() {
    ctx = new rs2.Context();
    const devices = ctx.queryDevices().devices;
    assert(devices.length > 0); // Device must be connected
    dev = devices[0];
  });

  afterEach(function() {
    rs2.cleanup();
  });

  it('Testing constructor', () => {
    assert.doesNotThrow(() => {
      new rs2.Device(dev);
    });
  });

  it('Testing member - first', () => {
    assert(dev.first instanceof rs2.Sensor);
  });

  it('Testing member - isValid', () => {
    assert.equal(typeof dev.isValid, 'boolean');
  });

  it('Testing method destroy', () => {
    assert.doesNotThrow(() => {
      dev.destroy();
    });
  });

  it('Testing method getCameraInfo - without argument', () => {
    assert.doesNotThrow(() => {
      dev.getCameraInfo();
    });
  });

  it('Testing method getCameraInfo - return value', () => {
    let rtn = dev.getCameraInfo();
    assert.equal(typeof rtn.name, 'string');
    assert.equal(typeof rtn.serialNumber, 'string');
    assert.equal(typeof rtn.firmwareVersion, 'string');
    assert.equal(typeof rtn.physicalPort, 'string');
    assert.equal(typeof rtn.debugOpCode, 'string');
    assert.equal(typeof rtn.advancedMode, 'string');
    assert.equal(typeof rtn.productId, 'string');
    // assert.equal(typeof rtn.cameraLocked, 'boolean'); // skip cameraLocked checking
  });

  it('Testing method getCameraInfo - with argument', () => {
    for (let i in rs2.camera_info) {
      if (rs2.camera_info[i] &&
        i.toUpperCase() !== 'CAMERA_INFO_LOCATION' && // location is not ready
        i.toUpperCase() !== 'CAMERA_INFO_COUNT' && // skip counter
        i.toUpperCase() !== 'CAMERA_INFO_CAMERA_LOCKED' &&
        i !== 'cameraInfoToString' // skip method
      ) {
        assert.doesNotThrow(() => { // jshint ignore:line
          dev.getCameraInfo(rs2.camera_info[i]);
        });
      }
    }
  });

  it('Testing method getCameraInfo - with invalid argument', () => {
    assert.throws(() => {
      dev.getCameraInfo('dummy');
    });
  });

  it('Testing method querySensors - return value', () => {
    assert.doesNotThrow(() => {
      dev.querySensors();
    });
    assert.equal(Object.prototype.toString.call(dev.querySensors()), '[object Array]');
    dev.querySensors().forEach((i) => {
      assert(dev.querySensors()[0] instanceof rs2.Sensor);
    });
  });

  it('Testing method reset', () => {
    assert.doesNotThrow(() => {
      dev.reset();
    });
    assert.equal(typeof dev.reset(), 'undefined');
  });

  it('Testing method supportsCameraInfo - without argument', () => {
    assert.throws(() => {
      dev.supportsCameraInfo();
    });
  });

  // BUG-7430
  it('Testing method supportsCameraInfo - return value', () => {
    let rtn = dev.supportsCameraInfo('serial-number');
    assert.equal(typeof rtn, 'boolean');
  });

  it('Testing method supportsCameraInfo - with invalid argument', () => {
    assert.throws(() => {
      dev.supportsCameraInfo('dummy');
    });
  });

  it('Testing method supportsCameraInfo - with argument', () => {
    for (let i in rs2.camera_info) {
      if (rs2.camera_info[i] &&
        i.toUpperCase() !== 'CAMERA_INFO_LOCATION' && // BUG-7429
        i.toUpperCase() !== 'CAMERA_INFO_COUNT' && // skip counter
        i !== 'cameraInfoToString' // skip method
        ) {
        assert.doesNotThrow(() => { // jshint ignore:line
          dev.supportsCameraInfo(rs2.camera_info[i]);
        });
      }
    }
  });
});
