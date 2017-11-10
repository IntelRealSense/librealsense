// Copyright (c) 2017 Intel Corporation. All rights reserved.
// Use of this source code is governed by an Apache 2.0 license
// that can be found in the LICENSE file.

'use strict';

/* global describe, it, before, after */
const assert = require('assert');
const rs2 = require('../index.js');

let ctx;
let depthSensor;
describe('Device test', function() {
  before(function() {
    ctx = new rs2.Context();
    const devices = ctx.queryDevices().devices;
    assert(devices.length > 0); // Device must be connected
    const dev = devices[0];
    const pipeline = new rs2.Pipeline();
    pipeline.start();
    while (!depthSensor) {
      const sensors = dev.querySensors();
      for (let i = 0; i < sensors.length; i++) {
        if (sensors[i] instanceof rs2.DepthSensor) {
          depthSensor = sensors[i];
        }
      }
    }
  });

  after(function() {
    rs2.cleanup();
  });

  it('Testing constructor', () => {
    assert.doesNotThrow(() => {
      new rs2.DepthFrame();
    });
  });

  it('Testing member depthScale - without argument', () => {
    let v;
    assert.doesNotThrow(() => {
      v = depthSensor.depthScale;
    });
    assert(typeof v !== undefined);
  });
});
