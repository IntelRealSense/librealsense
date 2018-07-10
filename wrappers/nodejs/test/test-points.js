// Copyright (c) 2017 Intel Corporation. All rights reserved.
// Use of this source code is governed by an Apache 2.0 license
// that can be found in the LICENSE file.

'use strict';

/* global describe, it, beforeEach, afterEach */
const assert = require('assert');
const fs = require('fs');
let rs2;
try {
  rs2 = require('node-librealsense');
} catch (e) {
  rs2 = require('../index.js');
}

let ctx;
let pipeline;
let pointcloud;
let frameSet;
let points;
let files = 'points.ply';
describe('Points test', function() {
  beforeEach(function() {
    ctx = new rs2.Context();
    const devices = ctx.queryDevices().devices;
    assert(devices.length > 0); // Device must be connected
  });

  afterEach(function() {
    pipeline.destroy();
    pipeline.stop();
    rs2.cleanup();
    frameSet.destroy();
    if (fs.existsSync(files)) {
      fs.unlinkSync(files);
    }
  });

  it('Testing member textureCoordinates', () => {
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
  });

  it('Testing member vertices', () => {
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
  });

  it('Testing member size', () => {
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
  });

  it('Testing method exportToPly without argument', () => {
    assert.doesNotThrow(() => {
      pipeline = new rs2.Pipeline();
      pointcloud = new rs2.PointCloud();
      pipeline.start();
      frameSet = pipeline.waitForFrames();
      points = pointcloud.calculate(frameSet.depthFrame);
    });
    assert.throws(() => {
      points.exportToPly();
    });
  });

  it('Testing method exportToPly with three arguments', () => {
    assert.doesNotThrow(() => {
      pipeline = new rs2.Pipeline();
      pointcloud = new rs2.PointCloud();
      pipeline.start();
      frameSet = pipeline.waitForFrames();
      points = pointcloud.calculate(frameSet.depthFrame);
    });
    assert.throws(() => {
      points.exportToPly(files, files, files);
    });
  });

  it('Testing method exportToPly first argument with number', () => {
    assert.doesNotThrow(() => {
      pipeline = new rs2.Pipeline();
      pointcloud = new rs2.PointCloud();
      pipeline.start();
      frameSet = pipeline.waitForFrames();
      points = pointcloud.calculate(frameSet.depthFrame);
    });
    assert.throws(() => {
      points.exportToPly(1, 1);
    });
  });

  it('Testing method exportToPly second argument with number', () => {
    assert.doesNotThrow(() => {
      pipeline = new rs2.Pipeline();
      pointcloud = new rs2.PointCloud();
      pipeline.start();
      frameSet = pipeline.waitForFrames();
      points = pointcloud.calculate(frameSet.depthFrame);
    });
    assert.throws(() => {
      points.exportToPly(files, 1);
    });
  });

  it('Testing method exportToPly with depthFrame', () => {
    assert.doesNotThrow(() => {
      pipeline = new rs2.Pipeline();
      pointcloud = new rs2.PointCloud();
      pipeline.start();
      frameSet = pipeline.waitForFrames();
    });
    assert.doesNotThrow(() => {
      points = pointcloud.calculate(frameSet.depthFrame);
      points.exportToPly(files, frameSet.depthFrame);
      assert.equal(fs.existsSync(files), true);
    });
  });

  it('Testing method exportToPly with colorFrame', () => {
    assert.doesNotThrow(() => {
      pipeline = new rs2.Pipeline();
      pointcloud = new rs2.PointCloud();
      pipeline.start();
      frameSet = pipeline.waitForFrames();
    });
    assert.doesNotThrow(() => {
      points = pointcloud.calculate(frameSet.depthFrame);
      points.exportToPly(files, frameSet.colorFrame);
      assert.equal(fs.existsSync(files), true);
    });
  });

  it('Testing method exportToPly with videoFrame', () => {
    assert.doesNotThrow(() => {
      pipeline = new rs2.Pipeline();
      pointcloud = new rs2.PointCloud();
      pipeline.start();
      frameSet = pipeline.waitForFrames();
      points = pointcloud.calculate(frameSet.depthFrame);
    });
    assert.doesNotThrow(() => {
      let videoFrame = frameSet.at(1);
      points.exportToPly(files, videoFrame);
      assert.equal(fs.existsSync(files), true);
    });
  });
});
