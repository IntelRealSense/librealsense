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
let pipeline;
let pipelineProfile;
let streamProfiles;

describe('VideoStreamProfile test', function() {
  before(function() {
    ctx = new rs2.Context();
    const devices = ctx.queryDevices().devices;
    assert(devices.length > 0); // Device must be connected
    pipeline = new rs2.Pipeline(ctx);
    pipeline.start();
    pipelineProfile = pipeline.getActiveProfile();
    streamProfiles = pipelineProfile.getStreams();
  });

  after(function() {
    pipeline.stop();
    pipeline.destroy();
    rs2.cleanup();
  });

  it('Testing constructor - 0 option', () => {
    assert.throws(() => {
      new rs2.StreamProfile();
    });
  });

  it('Testing member - width', () => {
    streamProfiles.forEach( (stream) => {
      assert.equal(typeof stream.width, 'number');
    });
  });

  it('Testing member - height', () => {
    streamProfiles.forEach( (stream) => {
      assert.equal(typeof stream.height, 'number');
    });
  });

  it('Testing member - streamIndex', () => {
    streamProfiles.forEach( (stream) => {
      assert.equal(typeof stream.streamIndex, 'number');
    });
  });

  it('Testing member - streamType', () => {
    streamProfiles.forEach( (stream) => {
      assert.equal(typeof stream.streamType, 'number');
    });
  });

  it('Testing member - format', () => {
    streamProfiles.forEach( (stream) => {
      assert.equal(typeof stream.format, 'number');
    });
  });

  it('Testing member - fps', () => {
    streamProfiles.forEach( (stream) => {
      assert.equal(typeof stream.fps, 'number');
    });
  });

  it('Testing member - uniqueID', () => {
    streamProfiles.forEach( (stream) => {
      assert.equal(typeof stream.uniqueID, 'number');
    });
  });

  it('Testing member - isDefault', () => {
    streamProfiles.forEach( (stream) => {
      assert.equal(typeof stream.isDefault, 'boolean');
    });
  });

  it('Testing mothod getExtrinsicsTo - 0 argument', () => {
    streamProfiles.forEach( (stream) => {
      assert.throws(() => {
        stream.getExtrinsicsTo();
      });
    });
  });

  it('Testing method getExtrinsicsTo - valid argument', () => {
    let ExtrinsicsObject;
    assert.doesNotThrow(() => {
      ExtrinsicsObject = streamProfiles[0].getExtrinsicsTo(streamProfiles[1]);
    });
    assert.equal(Object.prototype.toString.call(ExtrinsicsObject.rotation), '[object Array]');
  });

  it('Testing method getExtrinsicsTo - invalid argument', () => {
    assert.throws(() => {
      streamProfiles[0].getExtrinsicsTo('dummy');
    });
  });

  it('Testing method getIntrinsics - valid argument', () => {
    streamProfiles.forEach((stream) => {
      let intrinsics = stream.getIntrinsics();
      assert.equal(typeof intrinsics.width, 'number');
      assert.equal(typeof intrinsics.height, 'number');
      assert.equal(typeof intrinsics.ppx, 'number');
      assert.equal(typeof intrinsics.ppy, 'number');
      assert.equal(typeof intrinsics.fx, 'number');
      assert.equal(typeof intrinsics.fy, 'number');
      assert.equal(typeof intrinsics.model, 'number');
      assert.equal(Object.prototype.toString.call(intrinsics.coeffs), '[object Array]');
      assert.equal(intrinsics.coeffs.length, 5);
    });
  });

  it('Testing method getIntrinsics - invalid argument', () => {
    streamProfiles.forEach((stream) => {
      assert.doesNotThrow(() => {
        stream.getIntrinsics('dummy');
      });
    });
  });
});
