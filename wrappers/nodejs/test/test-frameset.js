// Copyright (c) 2018 Intel Corporation. All rights reserved.
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

let frameset;
let pipeline;
describe('FrameSet test', function() {
  before(function() {
    pipeline = new rs2.Pipeline();
    pipeline.start();
    while (frameset === undefined) {
      const f = pipeline.waitForFrames();
      if (f.size > 1) {
        frameset = f;
      }
    }
  });

  after(function() {
    pipeline.destroy();
    rs2.cleanup();
  });

  it('Testing constructor', () => {
    assert.doesNotThrow(() => {
      new rs2.FrameSet();
    });
  });

  it('Testing member size', () => {
    assert.equal(typeof frameset.size, 'number');
  });

  it('Testing member depthFrame', () => {
    assert(frameset.depthFrame instanceof rs2.DepthFrame);
  });

  it('Testing member colorFrame', () => {
    assert(frameset.colorFrame instanceof rs2.VideoFrame);
  });

  it('Testing method at - 0 argument', () => {
    assert.throws(() => {
      frameset.at();
    });
  });

  it('Testing method at - invalid argument', () => {
    const len = frameset.size;
    let f;
    assert.throws(() => {
      f = frameset.at(len + 1);
    });
    assert.equal(f, undefined);
  });

  it('Testing method at - valid argument', () => {
    const len = frameset.size;
    for (let i = 0; i < len; i++) {
      let f;
      assert.doesNotThrow(() => { // jshint ignore:line
        f = frameset.at(i);
      });
      assert(f instanceof rs2.DepthFrame || f instanceof rs2.VideoFrame);
    }
  });

  it('Testing method getFrame - 0 argument', () => {
    assert.throws(() => {
      frameset.getFrame();
    });
  });

  it('Testing method getFrame - invalid argument', () => {
    assert.throws(() => {
      frameset.getFrame('dummy');
    });
  });

  it('Testing method getFrame - valid argument', () => {
    for (let i in rs2.stream) {
      if (rs2.stream[i] &&
        i.toUpperCase() !== 'STREAM_COUNT' && // skip counter
        i !== 'streamToString' // skip method
      ) {
        assert.doesNotThrow(() => { // jshint ignore:line
          frameset.getFrame(rs2.stream[i]);
        });
        assert.doesNotThrow(() => { // jshint ignore:line
          frameset.getFrame(rs2.stream[i]);
        });
      }
    }
  });

  it('Testing method getFrame - stream_depth', () => {
    let d = frameset.getFrame(rs2.stream['stream_depth']); // jshint ignore:line
    assert(d instanceof rs2.DepthFrame);
    let D = frameset.getFrame(rs2.stream['STREAM_DEPTH']); // jshint ignore:line
    assert(D instanceof rs2.DepthFrame);
  });

  it('Testing method getFrame - stream_color', () => {
    let d = frameset.getFrame(rs2.stream['stream_color']); // jshint ignore:line
    assert(d instanceof rs2.VideoFrame);
    let D = frameset.getFrame(rs2.stream['STREAM_COLOR']); // jshint ignore:line
    assert(D instanceof rs2.VideoFrame);
  });

  it('Testing method forEach', () => {
    let counter = 0;
    function callback(frame) {
      counter++;
      assert.equal(frame instanceof rs2.Frame, true);
    }
    frameset.forEach(callback);
    assert.equal(counter, frameset.size);
  });
});
