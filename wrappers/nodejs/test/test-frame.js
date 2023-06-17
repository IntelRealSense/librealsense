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

let frame;
let pipeline;
describe('Frame test', function() {
  before(function() {
    pipeline = new rs2.Pipeline();
    pipeline.start();
    while (!frame) {
      const frameset = pipeline.waitForFrames();
      frame = frameset.at(0);
    }
  });

  after(function() {
    pipeline.destroy();
    rs2.cleanup();
  });

  it('Testing constructor', () => {
    assert.doesNotThrow(() => {
      new rs2.Frame();
    });
  });

  it('Testing property isValid', () => {
    assert.equal(typeof frame.isValid, 'boolean');
  });

  it('Testing property data', () => {
    assert(Object.prototype.toString.call(frame.data), '[object Uint16Array]' ||
      Object.prototype.toString.call(frame.data), '[object Uint8Array]'
    );
  });

  it('Testing property width', () => {
    assert.equal(typeof frame.width, 'number');
  });

  it('Testing property height', () => {
    assert.equal(typeof frame.height, 'number');
  });

  it('Testing property frameNumber', () => {
    assert.equal(typeof frame.frameNumber, 'number');
  });

  it('Testing property timestamp', () => {
    assert.equal(typeof frame.timestamp, 'number');
  });

  it('Testing property streamType', () => {
    assert.equal(typeof frame.streamType, 'number');
  });

  it('Testing property dataByteLength', () => {
    assert.equal(typeof frame.dataByteLength, 'number');
  });

  it('Testing property strideInBytes', () => {
    assert.equal(typeof frame.strideInBytes, 'number');
  });

  it('Testing property bitsPerPixel', () => {
    assert.equal(typeof frame.bitsPerPixel, 'number');
  });

  it('Testing property timestampDomain', () => {
    assert.equal(typeof frame.timestampDomain, 'number');
  });

  it('Testing method frameMetadata - 0 argument', () => {
    assert.throws(() => {
      frame.frameMetadata();
    });
  });

  it('Testing method frameMetadata - invalid argument', () => {
    assert.throws(() => {
      frame.frameMetadata('dummy');
    });
  });

  it('Testing method frameMetadata - valid argument', () => {
    for (let i in rs2.frame_metadata) {
      if (rs2.frame_metadata[i] &&
      i.toUpperCase() !== 'FRAME_METADATA_COUNT' && // skip counter
      i !== 'frameMetadataToString' // skip method
      ) {
        if (frame.supportsFrameMetadata(rs2.frame_metadata[i])) {
          assert.doesNotThrow(() => { // jshint ignore:line
            frame.frameMetadata(rs2.frame_metadata[i]);
          });
          assert.equal(Object.prototype.toString.call(
            frame.frameMetadata(rs2.frame_metadata[i])
          ), '[object Uint8Array]');
        } else {
          assert.throws(() => { // jshint ignore:line
            frame.frameMetadata(rs2.frame_metadata[i]);
          });
        }
      }
    }
  });

  it('Testing method getData - 0 argument', () => {
    assert.doesNotThrow(() => {
      frame.getData();
    });
    assert(
      Object.prototype.toString.call(frame.getData()), '[object Uint16Array]' ||
      Object.prototype.toString.call(frame.getData()), '[object Uint8Array]' ||
      Object.prototype.toString.call(frame.getData()), '[object Buffer]'
    );
  });

  it('Testing method getData - buffer argument', () => {
    const len = frame.dataByteLength;
    let buf = new ArrayBuffer(len);
    assert.doesNotThrow(() => {
      frame.getData(buf);
    });
    assert(
      Object.prototype.toString.call(buf), '[object Uint16Array]' ||
      Object.prototype.toString.call(buf), '[object Uint8Array]' ||
      Object.prototype.toString.call(buf), '[object Buffer]'
    );
  });

  it('Testing method getData - 2 argument', () => {
    assert.throws(() => {
      frame.getData(1, 2);
    });
  });

  it('Testing method getData - invalid argument', () => {
    assert.throws(() => {
      frame.getData('dummy');
    });
  });

  it('Testing method supportsFrameMetadata - invalid argument', () => {
    assert.throws(() => {
      frame.supportsFrameMetadata('dummy');
    });
  });

  it('Testing method supportsFrameMetadata - valid argument', () => {
    for (let i in rs2.frame_metadata) {
      if (rs2.frame_metadata[i] &&
      i.toUpperCase() !== 'FRAME_METADATA_COUNT' && // skip counter
      i !== 'frameMetadataToString' // skip method
      ) {
        assert.doesNotThrow(() => { // jshint ignore:line
          frame.supportsFrameMetadata(rs2.frame_metadata[i]);
        });
        assert.equal(Object.prototype.toString.call(
          frame.supportsFrameMetadata(rs2.frame_metadata[i])
        ), '[object Boolean]');
      }
    }
  });

  it('Testing method destroy', () => {
    assert.doesNotThrow(() => {
      frame.destroy();
    });
  });
});
