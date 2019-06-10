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
describe('Colorizer test', function() {
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
      new rs2.Colorizer();
    });
  });

  it('Testing constructor - 1 option', () => {
    assert.doesNotThrow(() => {
      new rs2.Colorizer(1);
    });
  });

  it('Testing method destroy', () => {
    let colorizer;
    assert.doesNotThrow(() => {
      colorizer= new rs2.Colorizer();
      colorizer.destroy();
    });
    assert.equal(colorizer.cxxColorizer, undefined);
    assert.equal(colorizer.depthRGB, undefined);
  });

  it('Testing method colorize', () => {
    let pipeline;
    let colorizer;
    assert.doesNotThrow(() => {
      pipeline = new rs2.Pipeline();
      colorizer = new rs2.Colorizer();
      pipeline.start();
    });
    let endTest = false;
    let n = 0;
    while (!endTest) {
      const frameSet = pipeline.waitForFrames();
      n++;
      console.log(`retring left ...${10 - n} times`);
      if (frameSet !== undefined &&
          frameSet.colorFrame !== undefined &&
          frameSet.depthFrame !== undefined) {
        const depthf = frameSet.depthFrame;
        const colorf = frameSet.colorFrame;
        assert(depthf instanceof rs2.DepthFrame);
        assert(colorf instanceof rs2.VideoFrame);
        let videoFrame;
        let videoFrameNull;
        assert.doesNotThrow(() => { // jshint ignore:line
          videoFrame = colorizer.colorize(depthf);
        });
        assert(videoFrame instanceof rs2.VideoFrame);
        assert.throws(() => { // jshint ignore:line
          videoFrameNull = colorizer.colorize();
        });
        assert(videoFrameNull === undefined);
        colorizer.destroy();
        if (colorf) colorf.destroy();
        if (depthf) depthf.destroy();
        endTest = true;
        if (videoFrame) videoFrame.destroy();
      }
      if (n >= 10) {
        assert(false, 'could not get colorFrame or depthFrame, try to reset camera');
      }
    }
    pipeline.destroy();
  }).timeout(4000);
});
