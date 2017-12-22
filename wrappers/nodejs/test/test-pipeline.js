// Copyright (c) 2017 Intel Corporation. All rights reserved.
// Use of this source code is governed by an Apache 2.0 license
// that can be found in the LICENSE file.

'use strict';

/* global describe, it, before, after */
const assert = require('assert');
let rs2;
try {
  rs2 = require('node-librealsense');
} catch (e) {
  rs2 = require('../index.js');
}

let ctx;
describe('Pipeline test', function() {
  before(function() {
    ctx = new rs2.Context();
    const devices = ctx.queryDevices().devices;
    assert(devices.length > 0); // Device must be connected
  });

  after(function() {
    rs2.cleanup();
  });

  it('Testing constructor - 0 option', () => {
    assert.doesNotThrow(() => {
      new rs2.Pipeline();
    });
  });

  it('Testing constructor - 2 options', () => {
    assert.throws(() => {
      new rs2.Pipeline(1, 2);
    });
  });

  it('Testing constructor - 1 invalid option', () => {
    assert.throws(() => {
      new rs2.Pipeline(1);
    });
  });

  it('Testing constructor - 1 context option', () => {
    assert.doesNotThrow(() => {
      new rs2.Pipeline(ctx);
    });
  });

  it('Testing method destroy', () => {
    let pipeline;
    assert.doesNotThrow(() => {
      pipeline = new rs2.Pipeline();
      pipeline.start();
      pipeline.destroy();
    });
  });


  it('Testing method waitForFrames', () => {
    let pipeline;
    let frameSet;
    assert.doesNotThrow(() => {
      pipeline = new rs2.Pipeline();
      pipeline.start();
      frameSet = pipeline.waitForFrames();
    });
    let endTest = false;
    let n = 0;
    while (!endTest) {
      frameSet = pipeline.waitForFrames();
      n++;
      console.log(`retring left ...${10 - n} times`);
      if (frameSet !== undefined && frameSet.colorFrame !== undefined &&
        frameSet.depthFrame !== undefined) {
        assert(frameSet.depthFrame instanceof rs2.VideoFrame);
        assert(frameSet.colorFrame instanceof rs2.VideoFrame);
        endTest = true;
      }
      if (n >= 10) {
        assert(false, 'could not get colorFrame or depthFrame, try to reset camera');
      }
    }
    pipeline.destroy();
  });

  it('Testing method start', () => {
    let pipeline;
    assert.doesNotThrow(() => {
      pipeline = new rs2.Pipeline();
      pipeline.start();
    });
    assert.notEqual(pipeline, undefined);
    assert.doesNotThrow(() => {
      pipeline.start();
      pipeline.destroy();
    });
  });

  it('Testing method stop', () => {
    let pipeline;
    pipeline = new rs2.Pipeline();
    assert.notEqual(pipeline, undefined);
    assert.doesNotThrow(() => {
      pipeline.start();
      pipeline.stop();
      pipeline.destroy();
   });
  });
});
