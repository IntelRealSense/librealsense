// Copyright (c) 2017 Intel Corporation. All rights reserved.
// Use of this source code is governed by an Apache 2.0 license
// that can be found in the LICENSE file.

'use strict';

/* global describe, it, before, after */
const assert = require('assert');
const rs2 = require('../index.js');
const RS2 = require('bindings')('node_librealsense');

let ctx;
let sensors;
describe('Sensor test', function() {
  before(function() {
    ctx = new rs2.Context();
    sensors = ctx.querySensors();
    assert(sensors.length > 0); // Sensor must be connected
  });

  after(function() {
    rs2.cleanup();
  });

  const optionsTestArray = [
      rs2.option.option_enable_auto_exposure,
      'enable_auto_exposure',
      rs2.option.OPTION_ENABLE_AUTO_WHITE_BALANCE,
      RS2.RS2_OPTION_ENABLE_AUTO_WHITE_BALANCE,
    ];

  it.skip('Testing method getMotionIntrinsics', () => {
    sensors.forEach((sensor) => {
      Object.keys(rs2.stream).forEach((o) => {
        if (o === 'STREAM_COUNT' || o === 'streamToString') return;
        const m = sensor.getMotionIntrinsics(rs2.stream[o]);
        assert.equal(typeof m, 'object');
        assert.equal(Object.prototype.toString.call(m.data), '[object Float32Array]');
        assert.equal(m.data.length, 12);
        assert.equal(Object.prototype.toString.call(m.noiseVariances), '[object Float32Array]');
        assert.equal(m.noiseVariances.length, 3);
        assert.equal(Object.prototype.toString.call(m.biasVariances), '[object Float32Array]');
        assert.equal(m.biasVariances.length, 3);
      });
    });
  });

  it('Testing method isOptionReadOnly', () => {
    sensors.forEach((sensor) => {
      Object.keys(rs2.option).forEach((o) => {
        if (o === 'OPTION_COUNT' || o === 'optionToString') return;
        if (sensor.supportsOption(rs2.option[o])) {
          assert.equal(typeof sensor.isOptionReadOnly(rs2.option[o]), 'boolean');
        } else {
          assert.equal(typeof sensor.isOptionReadOnly(rs2.option[o]), 'undefined');
        }
      });
      optionsTestArray.forEach((o) => {
        if (sensor.supportsOption(o)) {
          assert.equal(typeof sensor.isOptionReadOnly(o), 'boolean');
        } else {
          assert.equal(typeof sensor.isOptionReadOnly(o), 'undefined');
        }
      });
    });
  });

  it('Testing method isOptionReadOnly, invalid value', () => {
    sensors.forEach((sensor) => {
      assert.throws(() => {
        sensor.isOptionReadOnly('dummy');
      });
    });
  });

  it('Testing method supportsOption', () => {
    sensors.forEach((sensor) => {
      Object.keys(rs2.option).forEach((o) => {
        if (o === 'OPTION_COUNT' || o === 'optionToString') return;
        assert.equal(typeof sensor.supportsOption(rs2.option[o]), 'boolean');
      });
      optionsTestArray.forEach((o) => {
        assert.equal(typeof sensor.supportsOption(o), 'boolean');
      });
    });
  });

  it('Testing method supportsOption, invalid value', () => {
    sensors.forEach((sensor) => {
      assert.throws(() => {
        sensor.supportsOption('dummy');
      });
    });
  });

  it('Testing method supportsOption, null value', () => {
    sensors.forEach((sensor) => {
      assert.throws(() => {
        sensor.supportsOption();
      });
      assert.throws(() => {
        sensor.supportsOption(null);
      });
      assert.throws(() => {
        sensor.supportsOption(undefined);
      });
    });
  });

  it('Testing method getOption', () => {
    sensors.forEach((sensor) => {
      Object.keys(rs2.option).forEach((o) => {
        if (o === 'OPTION_COUNT' || o === 'optionToString') return;
        if (sensor.supportsOption(rs2.option[o])) {
          assert.equal(typeof sensor.getOption(rs2.option[o]), 'number');
        } else {
          assert.equal(typeof sensor.getOption(rs2.option[o]), 'undefined');
        }
      });
      optionsTestArray.forEach((o) => {
        if (sensor.supportsOption(o)) {
          assert.equal(typeof sensor.getOption(o), 'number');
        } else {
          assert.equal(typeof sensor.getOption(o), 'undefined');
        }
      });
    });
  });

  it('Testing method getOption, invalid value', () => {
    sensors.forEach((sensor) => {
      optionsTestArray.forEach((o) => {
        assert.throws(() => {
          sensor.getOption('dummy');
        });
        assert.throws(() => {
          sensor.getOption();
        });
        assert.throws(() => {
          sensor.getOption(null);
        });
        assert.throws(() => {
          sensor.getOption(undefined);
        });
      });
    });
  });

  it('Testing method getOptionRange', () => {
    sensors.forEach((sensor) => {
      Object.keys(rs2.option).forEach((o) => {
        if (o === 'OPTION_COUNT' || o === 'optionToString') return;
        if (sensor.supportsOption(rs2.option[o])) {
          const r = sensor.getOptionRange(rs2.option[o]);
          assert.equal(typeof r, 'object');
          assert.equal(typeof r.minValue, 'number');
          assert.equal(typeof r.maxValue, 'number');
          assert.equal(typeof r.defaultValue, 'number');
          assert.equal(typeof r.step, 'number');
        } else {
          assert.equal(typeof sensor.getOptionRange(rs2.option[o]), 'undefined');
        }
      });
      optionsTestArray.forEach((o) => {
        if (sensor.supportsOption(o)) {
          const r = sensor.getOptionRange(o);
          assert.equal(typeof r, 'object');
          assert.equal(typeof r.minValue, 'number');
          assert.equal(typeof r.maxValue, 'number');
          assert.equal(typeof r.defaultValue, 'number');
          assert.equal(typeof r.step, 'number');
        } else {
          assert.equal(typeof sensor.getOptionRange(o), 'undefined');
        }
      });
    });
  });

  it('Testing method setOption', () => {
    sensors.forEach((sensor) => {
      optionsTestArray.forEach((o) => {
        assert.doesNotThrow(() => {
          sensor.setOption(o, 10);
        });
      });
      Object.keys(rs2.option).forEach((o) => {
        if (o === 'OPTION_COUNT' || o === 'optionToString') return;
        // retrun if sensor does not support option
        if (!sensor.supportsOption(rs2.option[o])) return;
        if (rs2.option[o] === 'visual-preset' ||
          rs2.option[o] === 'error-polling-enabled' ||
          rs2.option[o] === 'output-trigger-enabled' ||
          rs2.option[o] === 'depth-units' ||
          rs2.option[o] === 12 ||
          rs2.option[o] === 24 ||
          rs2.option[o] === 26 ||
          rs2.option[o] === 28 ||
          rs2.option[o] === 'backlight-compensation' ||
          rs2.option[o] === 'white-balance' ||
          rs2.option[o] === 'enable-auto-white-balance' ||
          rs2.option[o] === 'power-line-frequency' ||
          rs2.option[o] === 0 ||
          rs2.option[o] === 9 ||
          rs2.option[o] === 11 ||
          rs2.option[o] === 22 ||
          rs2.option[o] === 19
          ) return;
        const v = sensor.getOption(rs2.option[o]);
        sensor.setOption(rs2.option[o], v + 1);
        const vNew = sensor.getOption(rs2.option[o]);
        assert.equal(vNew, v + 1);
      });
    });
  }).timeout(20 * 1000);

  it.skip('Testing method getOptionDescription', () => {
    sensors.forEach((sensor) => {
      optionsTestArray.forEach((o) => {
        assert.equal(typeof sensor.getOptionDescription(o), 'string');
      });
    });
  });

  it.skip('Testing method getOptionValueDescription', () => {
    sensors.forEach((sensor) => {
      optionsTestArray.forEach((o) => {
        assert.equal(typeof sensor.getOptionValueDescription(o), 'string');
      });
    });
  });

  it('Testing method isValid', () => {
    sensors.forEach((sensor) => {
      assert.doesNotThrow(() => {
        sensor.isValid();
      });
    });
  });

  it('Testing method isValid, return boolean', () => {
    sensors.forEach((sensor) => {
      assert.equal(typeof sensor.isValid(), 'boolean');
    });
  });

  it('Testing method getStreamProfiles', () => {
    sensors.forEach((sensor) => {
      let profiles;
      assert.doesNotThrow(() => {
        profiles = sensor.getStreamProfiles();
      });
      /* eslint-disable guard-for-in */
      for (let i in profiles) {
        assert.equal(typeof profiles[i].format, 'number');
        assert.equal(typeof profiles[i].fps, 'number');
        assert.equal(typeof profiles[i].isDefault, 'boolean');
        assert.equal(typeof profiles[i].streamIndex, 'number');
        assert.equal(typeof profiles[i].streamType, 'number');
        assert.equal(typeof profiles[i].uniqueID, 'number');
      }
    });
  });

  it('Testing method open, streamProfile', () => {
    sensors.forEach((sensor) => {
      const profiles = sensor.getStreamProfiles();
      for (let i in profiles) {
        assert.doesNotThrow(() => { // jshint ignore:line
          sensor.open(profiles[i]);
        });
      }
    });
  });

  it.skip('Testing method open, profileArray', () => {
    sensors.forEach((sensor) => {
      const profiles = sensor.getStreamProfiles();
      for (let i in profiles) {
        const format = profiles[i].format;
        const fps = profiles[i].fps;
        const isDefault = profiles[i].isDefault;
        const streamIndex = profiles[i].streamIndex;
        const streamType = profiles[i].streamType;
        const uniqueID = profiles[i].uniqueID;
        console.log(profiles[i]);
        console.log(format, fps, isDefault, streamIndex, streamType, uniqueID);
        assert.doesNotThrow(() => { // jshint ignore:line
          sensor.open([format, fps, isDefault, streamIndex,
            streamType, uniqueID]);
        });
      }
    });
  });

  it.skip('Testing method start, with callback', () => {
    sensors.forEach((sensor) => {
      const profiles = sensor.getStreamProfiles();
      for (let i in profiles) {
        assert.doesNotThrow(() => { // jshint ignore:line
          sensor.open(profiles[i]);
        });
        sensor.start((frame) => { // jshint ignore:line
          assert.equal(typeof frame, 'object');
          assert.equal(typeof frame.isValid, 'boolean');
          assert.equal(Object.prototype.toString.call(frame.data), '[object Uint16Array]');
          assert.equal(typeof frame.width, 'number');
          assert.equal(typeof frame.height, 'number');
          assert.equal(typeof frame.frameNumber, 'number');
          assert.equal(typeof frame.timestamp, 'number');
          assert.equal(typeof frame.streamType, 'number');
          assert.equal(typeof frame.dataByteLength, 'number');
          assert.equal(typeof frame.strideInBytes, 'number');
          assert.equal(typeof frame.bitsPerPixel, 'number');
          assert.equal(typeof frame.timestampDomain, 'string');
        });
        sensor.stop();
      }
    });
  });
});
