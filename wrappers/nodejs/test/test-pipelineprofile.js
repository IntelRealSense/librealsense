// Copyright (c) 2018 Intel Corporation. All rights reserved.
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
let pipeline;
let pipelineProfile;
let devices;

describe('PipelineProfile test', function() {
  beforeEach(function() {
    ctx = new rs2.Context();
    devices = ctx.queryDevices().devices;
    assert(devices.length > 0); // Device must be connected
    pipeline = new rs2.Pipeline(ctx);
    pipeline.start();
    pipelineProfile = pipeline.getActiveProfile();
  });

  afterEach(function() {
    pipeline.stop();
    pipeline.destroy();
    rs2.cleanup();
  });

  it('Testing constructor - valid', () => {
    assert.doesNotThrow(() => {
      new rs2.PipelineProfile();
    });
  });

  it('Testing member isValid', () => {
    let res;
    assert.doesNotThrow(() => {
      res = pipelineProfile.isValid;
    });
    assert.equal(typeof res, 'boolean');
    assert(typeof res !== undefined);
  });

  it('Testing method destroy', () => {
    assert.doesNotThrow(() => {
      let res = pipelineProfile.destroy();
      assert.equal(typeof res, 'undefined');
    });
    assert.equal(pipelineProfile.cxxPipelineProfile, undefined);
  });

  it('Testing method getDevice', () => {
    assert.doesNotThrow(() => {
      let res = pipelineProfile.getDevice();
      assert(res instanceof rs2.Device);
      assert.equal(res.isValid, true);
    });
  });

  it('Testing method getDevice before pipeline.start', () => {
    pipeline.destroy();
    let pipe = new rs2.Pipeline(ctx);
    let pipeProfile = pipe.getActiveProfile();
    assert.throws(() => {
      pipeProfile.getDevice();
    });
    assert.doesNotThrow(() => {
      pipe.start();
      pipeProfile = pipe.getActiveProfile();
      let res = pipeProfile.getDevice();
      assert(res instanceof rs2.Device);
      assert.equal(res.isValid, true);
    });
  });

  it('Testing method getStream w/o args', () => {
    assert.throws(() => {
      pipelineProfile.getStream();
    });
  });

  it('Testing method getStream w/ invalid args', () => {
    assert.throws(() => {
      pipelineProfile.getStream('dummy');
    });
  });

  it('Testing method getStream w/ invalid number', () => {
    assert.throws(() => {
      pipelineProfile.getStream(999);
    });
  });

  it('Testing method getStream, w/ three args', () => {
    assert.throws(() => {
      pipelineProfile.getStream(1, 1, 1);
    });
  });

  it('Testing method getStream, w/ invalid streamIndex', () => {
    assert.doesNotThrow(() => {
      let res = pipelineProfile.getStream(1, 999);
      assert.equal(res, undefined);
    });
  });

  it('Testing method getStream, w/ one args', () => {
    let res = pipelineProfile.getStreams();
    let valueArray = [];
    res.forEach((s) => {
      if (valueArray.indexOf(s.streamIndex) === -1) valueArray.push(s.streamIndex);
    });
    valueArray.forEach((value) => {
      assert.doesNotThrow(() => {
        pipelineProfile.getStream(value);
      });
    });
  });

  it('Testing method getStreams', () => {
    assert.doesNotThrow(() => {
      let res = pipelineProfile.getStreams();
      assert.equal(Object.prototype.toString.call(res), '[object Array]');
      assert.notEqual(res, undefined);
      res.forEach((s) => {
        assert.equal(typeof s.format, 'number');
        assert.equal(typeof s.fps, 'number');
        assert.equal(typeof s.isDefault, 'boolean');
        assert.equal(typeof s.streamIndex, 'number');
        assert.equal(typeof s.streamType, 'number');
        assert.equal(typeof s.uniqueID, 'number');
      });
    });
  });
});

