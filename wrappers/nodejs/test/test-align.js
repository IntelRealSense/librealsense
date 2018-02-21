// Copyright (c) 2017 Intel Corporation. All rights reserved.
// Use of this source code is governed by an Apache 2.0 license
// that can be found in the LICENSE file.

'use strict';

/* global describe, it, before, after */
const assert = require('assert');
let rs2;
let pipeline;
try {
  rs2 = require('node-librealsense');
} catch (e) {
  rs2 = require('../index.js');
}

let frameset;
describe('Align test', function() {
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

  it('Testing constructor - 0 argument', () => {
    assert.throws(() => {
      new rs2.Align();
    });
  });

  it('Testing constructor - invalid argument', () => {
    assert.throws(() => {
      new rs2.Align('dummy');
    });
  });

  it('Testing constructor - valid argument', () => {
    for (let i in rs2.stream) {
      if (rs2.stream[i] &&
        i.toUpperCase() !== 'STREAM_COUNT' && // skip counter
        i !== 'streamToString' // skip method
      ) {
        assert.doesNotThrow(() => { // jshint ignore:line
          new rs2.Align(rs2.stream[i]);
        });
      }
    }
  });

  it('Testing process - 0 argument', () => {
    const align = new rs2.Align(rs2.stream.STREAM_COLOR);
    assert.throws(() => {
      align.process();
    });
  });

  it('Testing process - invalid argument', () => {
    const align = new rs2.Align(rs2.stream.STREAM_COLOR);
    assert.throws(() => {
      align.process('dummy');
    });
  });

  it('Testing process - valid argument', () => {
    const align = new rs2.Align(rs2.stream.STREAM_COLOR);
    assert.doesNotThrow(() => {
      align.process(frameset);
    });
  });
});
