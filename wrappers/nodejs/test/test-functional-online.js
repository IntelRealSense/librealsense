// Copyright (c) 2018 Intel Corporation. All rights reserved.
// Use of this source code is governed by an Apache 2.0 license
// that can be found in the LICENSE file.

'use strict';

/* global describe, it, before, after */
const assert = require('assert');
const fs = require('fs');
let rs2;
try {
  rs2 = require('node-librealsense');
} catch (e) {
  rs2 = require('../index.js');
}

describe('Disparity transform tests', function() {
  let ctx;
  let pipe;
  let depth2Disparity;
  let disparith2Depth;
  before(function() {
    ctx = new rs2.Context();
    pipe = new rs2.Pipeline(ctx);
    depth2Disparity = new rs2.DepthToDisparityTransform();
    disparith2Depth = new rs2.DisparityToDepthTransform();
  });

  after(function() {
    rs2.cleanup();
  });

  it('depth frame <=> disparity frame using transform', () => {
    pipe.start();
    const frames = pipe.waitForFrames();
    assert.equal(frames.size > 0, true);
    frames.forEach((frame) => {
      if (frame instanceof rs2.DepthFrame) {
        let output = depth2Disparity.process(frame);
        assert.equal(output instanceof rs2.DisparityFrame, true);
        assert.equal(typeof output.baseLine, 'number');

        let dout = disparith2Depth.process(output);
        assert.equal(dout instanceof rs2.DepthFrame, true);
      }
    });
    pipe.stop();
  });
});

describe('Points.exportToPly', function() {
  it('exportToPly', () => {
    const pc = new rs2.PointCloud();
    const pipe = new rs2.Pipeline();
    const file = 'points.ply';
    pipe.start();
    const frameset = pipe.waitForFrames();
    const points = pc.calculate(frameset.depthFrame);
    points.exportToPly(file, frameset.colorFrame);
    assert.equal(fs.existsSync(file), true);
    fs.unlinkSync(file);
    pipe.stop();
    rs2.cleanup();
  });
});
