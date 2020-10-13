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

describe('StreamProfile test', function() {
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

  it('Testing method getExtrinsicsTo - 0 argument', () => {
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

  it('Testing method streamToString - with stream number', () => {
    const streams = [
      'any',
      'depth',
      'color',
      'infrared',
      'fisheye',
      'gyro',
      'accel',
      'gpio',
      'pose',
      'confidence',
    ];
    assert.doesNotThrow(() => {
      for (let i = rs2.stream.STREAM_ANY; i < rs2.stream.STREAM_COUNT; i++) {
        let res = rs2.stream.streamToString(i);
        assert.equal(res, streams[i]);
      }
    });
  });

  it('Testing method streamToString - with two arguments', () => {
    assert.throws(() => {
      rs2.stream.streamToString(1, 1);
    });
  });

  it('Testing method streamToString - without argument', () => {
    assert.throws(() => {
      rs2.stream.streamToString();
    });
  });

  it('Testing method streamToString - with stream string', () => {
    const streams = [
      'stream_any',
      'stream_depth',
      'stream_color',
      'stream_infrared',
      'stream_fisheye',
      'stream_gyro',
      'stream_accel',
      'stream_gpio',
      'stream_pose',
      'stream_confidence',
    ];
    streams.forEach((s) => {
      assert.doesNotThrow(() => {
        rs2.stream.streamToString(rs2.stream[s]);
      });
    });
  });
});
