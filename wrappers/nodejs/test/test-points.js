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
describe('Points test', function() {
  before(function() {
    ctx = new rs2.Context();
    const devices = ctx.queryDevices().devices;
    assert(devices.length > 0); // Device must be connected
  });

  after(function() {
    rs2.cleanup();
  });

  it('Testing member textureCoordinates', () => {
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
        let points;
        let arr;
        assert.doesNotThrow(() => { // jshint ignore:line
          points = pointcloud.calculate(frameSet.depthFrame);
          arr = points.textureCoordinates;
        });
        assert.equal(typeof arr[0], 'number');
        assert.equal(Object.prototype.toString.call(arr), '[object Int32Array]');
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

  it('Testing member vertices', () => {
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
        let points;
        let arr;
        assert.doesNotThrow(() => { // jshint ignore:line
          points = pointcloud.calculate(frameSet.depthFrame);
          arr = points.vertices;
        });
        assert.equal(typeof arr[0], 'number');
        assert.equal(Object.prototype.toString.call(arr), '[object Float32Array]');
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

  it('Testing member size', () => {
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
        let points;
        let arr;
        assert.doesNotThrow(() => { // jshint ignore:line
          points = pointcloud.calculate(frameSet.depthFrame);
          arr = points.size;
        });
        assert.equal(typeof arr, 'number');
        assert.equal(Object.prototype.toString.call(arr), '[object Number]');
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
