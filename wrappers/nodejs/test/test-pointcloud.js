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
describe('Pointcloud test', function() {
  before(function() {
    ctx = new rs2.Context();
    const devices = ctx.queryDevices().devices;
    assert(devices.length > 0); // Device must be connected
  });

  after(function() {
    rs2.cleanup();
  });

  it('Testing method destroy', () => {
    let pointcloud;
    assert.doesNotThrow(() => {
      pointcloud= new rs2.PointCloud();
      pointcloud.destroy();
    });
    setTimeout(() => {
      assert.equal(pointcloud.pointsFrame, undefined);
      assert.equal(pointcloud.cxxPointCloud, undefined);
    }, 100);
  });

  it('Testing method calculate', () => {
    let pipeline;
    let frameSet;
    let pointcloud;
    assert.doesNotThrow(() => {
      pipeline = new rs2.Pipeline();
      pointcloud = new rs2.PointCloud();
      pipeline.start();
    });
    let endTest = false;
    let n = 0;
    while (!endTest) {
      frameSet = pipeline.waitForFrames();
      n++;
      console.log(`retring left ...${10 - n} times`);
      if (frameSet !== undefined && frameSet.colorFrame !== undefined &&
        frameSet.depthFrame !== undefined) {
        assert(frameSet.depthFrame instanceof rs2.DepthFrame);
        assert(frameSet.colorFrame instanceof rs2.VideoFrame);
        let points;
        let pointsNull;
        assert.doesNotThrow(() => { // jshint ignore:line
          points = pointcloud.calculate(frameSet.depthFrame);
        });
        assert(points instanceof rs2.Points);
        assert.throws(() => { // jshint ignore:line
          pointsNull = pointcloud.calculate();
        });
        assert(pointsNull === undefined);
        pointcloud.destroy();
        endTest = true;
      }
      if (n >= 10) {
        assert(false, 'could not get colorFrame or depthFrame, try to reset camera');
      }
    }
    frameSet.destroy();
    pipeline.destroy();
  });

  it('Testing method mapTo', () => {
    let pipeline;
    let frameSet;
    let pointcloud;
    assert.doesNotThrow(() => {
      pipeline = new rs2.Pipeline();
      pointcloud = new rs2.PointCloud();
      pipeline.start();
    });
    let endTest = false;
    let n = 0;
    while (!endTest) {
      frameSet = pipeline.waitForFrames();
      n++;
      console.log(`retring left ...${10 - n} times`);
      if (frameSet !== undefined && frameSet.colorFrame !== undefined &&
        frameSet.depthFrame !== undefined) {
        let rtn;
        let rtnNull;
        assert.doesNotThrow(() => { // jshint ignore:line
          rtn = pointcloud.mapTo(frameSet.colorFrame);
        });
        assert(rtn === undefined);
        assert.throws(() => { // jshint ignore:line
          rtnNull = pointcloud.mapTo();
        });
        assert(rtnNull === undefined);
        pointcloud.destroy();
        endTest = true;
      }
      if (n >= 10) {
        assert(false, 'could not get colorFrame or depthFrame, try to reset camera');
      }
    }
    frameSet.destroy();
    pipeline.destroy();
  });
});
