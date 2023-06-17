// Copyright (c) 2017 Intel Corporation. All rights reserved.
// Use of this source code is governed by an Apache 2.0 license
// that can be found in the LICENSE file.

'use strict';

/* global describe, it, before, after, beforeEach, afterEach */
const assert = require('assert');
let rs2;
try {
  rs2 = require('node-librealsense');
} catch (e) {
  rs2 = require('../index.js');
}

let ctx;
let pipeline;
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
});

describe('Pipeline method test', () => {
  beforeEach(() => {
    ctx = new rs2.Context();
    const devices = ctx.queryDevices().devices;
    assert(devices.length > 0); // Device must be connected
    pipeline = new rs2.Pipeline();
  });
  afterEach(() => {
    rs2.cleanup();
    pipeline.stop();
    pipeline.destroy();
  });

  it('Testing method destroy', () => {
    assert.doesNotThrow(() => {
      pipeline.start();
      pipeline.waitForFrames();
      pipeline.stop();
    });
  });

  it('Testing method destroy w/o start', () => {
    assert.doesNotThrow(() => {
      pipeline.destroy();
    });
    assert.equal(pipeline.ctx, undefined);
    assert.equal(pipeline.cxxPipeline, undefined);
    assert.equal(pipeline.started, false);
    assert.equal(pipeline.frameSet, undefined);
  });

  it('Testing method destroy after restart pipeline', () => {
    pipeline.start();
    pipeline.waitForFrames();
    assert.doesNotThrow(() => {
      pipeline.destroy();
    });
    assert.throws(() => {
      pipeline.start();
    });
  });

  it('Testing method waitForFrames', () => {
    let frameSet;
    assert.doesNotThrow(() => {
      pipeline.start();
      frameSet = pipeline.waitForFrames();
    });
    let endTest = false;
    let n = 0;
    while (!endTest) {
      frameSet = pipeline.waitForFrames();
      n++;
      if (n > 1) console.log(`retring left ...${10 - n} times`);
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
    pipeline.stop();
  });

  it('Testing method start', () => {
    assert.doesNotThrow(() => {
      let res = pipeline.start();
      pipeline.waitForFrames();
      assert(res instanceof rs2.PipelineProfile);
    });
    assert.notEqual(pipeline, undefined);
    assert.doesNotThrow(() => {
      pipeline.start();
      pipeline.waitForFrames();
      pipeline.stop();
    });
    assert.equal(pipeline.started, false);
  });

  it('Testing method start w/ invalid args', () => {
    assert.throws(() => {
      pipeline.start('dummy');
    });
    pipeline.stop();
  });

  it('Testing method start after stop', () => {
    assert.doesNotThrow(() => {
      pipeline.start();
      assert.equal(pipeline.started, true);
    });
    assert.doesNotThrow(() => {
      pipeline.stop();
      assert.equal(pipeline.started, false);
    });
    assert.doesNotThrow(() => {
      pipeline.start();
      assert.equal(pipeline.started, true);
    });
  });

  it('Testing method start after destroy', () => {
    assert.doesNotThrow(() => {
      pipeline.start();
      assert.equal(pipeline.started, true);
    });
    assert.doesNotThrow(() => {
      pipeline.destroy();
      assert.equal(pipeline.started, false);
    });
    assert.throws(() => {
      pipeline.start();
      assert.equal(pipeline.started, false);
    });
  });

  it('Testing method start w/ config', () => {
    let config = new rs2.Config();
    assert.doesNotThrow(() => {
      let res = pipeline.start(config);
      assert(res instanceof rs2.PipelineProfile);
    });
    assert.equal(pipeline.started, true);
    pipeline.stop();
  });

  it('Testing method start after started', () => {
    assert.doesNotThrow(() => {
      pipeline.start();
      assert.equal(pipeline.started, true);
      let res = pipeline.start();
      assert.equal(typeof res, 'undefined');
      assert.equal(pipeline.started, true);
    });
  });

  it('Testing method stop', () => {
    assert.notEqual(pipeline, undefined);
    assert.doesNotThrow(() => {
      pipeline.start();
      pipeline.waitForFrames();
      pipeline.stop();
   });
  });

  it('Testing method stop and restart pipeline', () => {
    assert.notEqual(pipeline, undefined);
    assert.doesNotThrow(() => {
      pipeline.start();
      pipeline.waitForFrames();
      pipeline.stop();
    });
    assert.equal(pipeline.started, false);
    assert.doesNotThrow(() => {
      pipeline.start();
    });
    assert.equal(pipeline.started, true);
    pipeline.stop();
  });

  it('Testing method stop when not start', () => {
    assert.notEqual(pipeline, undefined);
    assert.doesNotThrow(() => {
      let res = pipeline.stop();
      assert(typeof res, 'undefined');
    });
    assert.equal(pipeline.started, false);
  });

  it('Testing method getActiveProfile', () => {
    pipeline.start();
    assert.doesNotThrow(() => {
      let res = pipeline.getActiveProfile();
      assert(typeof res, 'object');
    });
    pipeline.stop();
  });

  it('Testing method getActiveProfile before pipe start', () => {
    assert.doesNotThrow(() => {
      let res = pipeline.getActiveProfile();
      assert(typeof res, 'undefined');
      pipeline.start();
      pipeline.waitForFrames();
      pipeline.stop();
    });
  });

  it('Testing method getActiveProfile after pipe stop', () => {
    assert.doesNotThrow(() => {
      pipeline.start();
      pipeline.waitForFrames();
      pipeline.stop();
      let res = pipeline.getActiveProfile();
      assert(typeof res, 'undefined');
    });
  });

  it('Testing method pollForFrames', () => {
    let then = Date.now();
    pipeline.start();
    pipeline.waitForFrames();
    let res;
    assert.doesNotThrow(() => {
      while (Date.now() < then + 2000) {
        res = pipeline.pollForFrames();
        if (res) {
          assert(typeof res.size, 'number');
          assert(typeof res.depthFrame, 'object');
          assert(typeof res.colorFrame, 'object');
          break;
        }
      }
    });
  });
});
