// Copyright (c) 2017 Intel Corporation. All rights reserved.
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
let sensors;
describe('Sensor test', function() {
  beforeEach(function() {
    ctx = new rs2.Context();
    sensors = ctx.querySensors();
    assert(sensors.length > 0); // Sensor must be connected
  });

  afterEach(function() {
    sensors.forEach((sensor) => {
      sensor.stop();
    });
    rs2.cleanup();
  });

  const optionsTestArray = Object.values(rs2.option);
  optionsTestArray.pop();
  optionsTestArray.pop();
  it('Testing member - isValid', () => {
    sensors.forEach((sensor) => {
      assert.equal(typeof sensor.isValid, 'boolean');
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
        const v = sensor.getOption(rs2.option[o]);
        let vSet;
        const r = sensor.getOptionRange(rs2.option[o]);
        if ((v - r.step) < r.minValue) {
          vSet = v + r.step;
        } else {
          vSet = v - r.step;
        }
        sensor.setOption(rs2.option[o], vSet);
        const vNew = sensor.getOption(rs2.option[o]);
        if (o.toUpperCase() === 'OPTION_DEPTH_UNITS') {
          assert.equal(Math.round(vNew * 100000)/100000, Math.round(vSet * 100000)/100000);
        } else {
          assert.equal(vNew, vSet);
        }
      });
    });
  }).timeout(30 * 1000);

  it('Testing method getOptionDescription', () => {
    sensors.forEach((sensor) => {
      optionsTestArray.forEach((o) => {
        assert(typeof sensor.getOptionDescription(o) === 'string' ||
          typeof sensor.getOptionDescription(o) === 'undefined'
        );
      });
    });
  });

  it('Testing method getOptionValueDescription', () => {
    sensors.forEach((sensor) => {
      optionsTestArray.forEach((o) => {
      let value;
      if (sensor.supportsOption(o)) {
        value = sensor.getOption(o);
      } else {
        value = 0;
      }
      assert(typeof sensor.getOptionValueDescription(o, value) === 'undefined' ||
        typeof sensor.getOptionValueDescription(o, value) === 'string'
      );
      });
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

  it('Testing method open, profileArray', () => {
    sensors.forEach((sensor) => {
      const profiles = sensor.getStreamProfiles();
        assert.doesNotThrow(() => { // jshint ignore:line
          sensor.open(profiles);
        });
    });
  });

  it('Testing method open, w/ two args', () => {
    sensors.forEach((sensor) => {
      const profiles = sensor.getStreamProfiles();
      for (let i in profiles) {
        assert.throws(() => { // jshint ignore:line
          sensor.open(profiles[i], profiles);
        });
      }
    });
  });

  it('Testing method open, w/o args', () => {
    sensors.forEach((sensor) => {
      assert.throws(() => {
        sensor.open();
      });
    });
  });

  it('Testing method open, w/ invaild args', () => {
    sensors.forEach((sensor) => {
      assert.throws(() => {
        sensor.open('dummy');
      });
    });
  });

  it('Testing method close check reopen', () => {
    sensors.forEach((sensor) => {
      const profiles = sensor.getStreamProfiles();
      assert.doesNotThrow(() => {
        sensor.close();
      });
      assert.doesNotThrow(() => {
        sensor.open(profiles);
      });
    });
  });

  it('Testing method destroy check reopen', () => {
    sensors.forEach((sensor) => {
      const profiles = sensor.getStreamProfiles();
      assert.doesNotThrow(() => {
        sensor.destroy();
        assert.equal(sensor.cxxSensor, undefined);
        assert.equal(sensor._events, null);
      });
      assert.throws(() => {
        sensor.open(profiles);
      });
    });
  });

  it('Testing method start, with callback', () => {
    let promises = [];
    sensors.forEach((sensor) => {
      let promise = new Promise((resolve) => {
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
            assert.equal(typeof frame.timestampDomain, 'number');
            sensor.stop();
            resolve();
          });
        }
      });
      promises.push(promise);
    });
    return Promise.all(promises);
  });

  it('Testing method start, w/ syncer', () => {
    let syncer = new rs2.Syncer();
    sensors.forEach((sensor) => {
      const profiles = sensor.getStreamProfiles();
      for (let i in profiles) {
        assert.doesNotThrow(() => { // jshint ignore:line
          sensor.open(profiles[i]);
        });
        sensor.start(syncer);
      }
    });
  });

  it('Testing method close', () => {
    sensors.forEach((sensor) => {
      assert.doesNotThrow(() => {
        sensor.close();
      });
    });
  });

  it('Testing method destroy', () => {
    sensors.forEach((sensor) => {
      assert.doesNotThrow(() => {
        sensor.destroy();
      });
    });
  });

  it('Testing method setNotificationsCallback notification', () => {
    return new Promise((resolve, reject) => {
      let dev = ctx.queryDevices().devices[0];
      sensors[0].setNotificationsCallback((n) => {
        assert.equal(typeof n.descr, 'string');
        assert.equal(typeof n.timestamp, 'number');
        assert.equal(typeof n.severity, 'string');
        assert.equal(typeof n.category, 'string');
        resolve();
      });
      setTimeout(() => {
        dev.cxxDev.triggerErrorForTest();
      }, 100);
    });
  });

  it('Testing method setNotificationsCallback w/o args', () => {
    sensors.forEach((sensor) => {
      assert.throws(() => {
        sensor.setNotificationsCallback();
      });
    });
  });

  it('Testing method setNotificationsCallback w/ invaild args', () => {
    sensors.forEach((sensor) => {
      assert.throws(() => {
        sensor.setNotificationsCallback('dummy');
      });
    });
  });

  it('Testing method supportsCameraInfo w/ number', () => {
    sensors.forEach((sensor) => {
      assert.doesNotThrow(() => {
        for (let i = rs2.camera_info.CAMERA_INFO_NAME; i < rs2.camera_info.CAMERA_INFO_COUNT; i++) {
          let obj = sensor.supportsCameraInfo(i);
          assert.equal(typeof obj, 'boolean');
        }
      });
    });
  });

  it('Testing method getCameraInfo w/ number', () => {
    sensors.forEach((sensor) => {
      assert.doesNotThrow(() => {
        for (let i = rs2.camera_info.CAMERA_INFO_NAME; i < rs2.camera_info.CAMERA_INFO_COUNT; i++) {
          if (sensor.supportsCameraInfo(i)) {
            let obj = sensor.getCameraInfo(i);
            assert.equal(typeof obj, 'string');
          }
        }
      });
    });
  });

  it('Testing method getCameraInfo w/ string', () => {
    sensors.forEach((sensor) => {
      assert.doesNotThrow(() => {
        for (let i = rs2.camera_info.CAMERA_INFO_NAME; i < rs2.camera_info.CAMERA_INFO_COUNT; i++) {
          let info = rs2.camera_info.cameraInfoToString(i);
          if (sensor.supportsCameraInfo(info)) {
            let obj = sensor.getCameraInfo(info);
            assert.equal(typeof obj, 'string');
          }
        }
      });
    });
  });

  it('Testing method supportsCameraInfo w/ string', () => {
    sensors.forEach((sensor) => {
      assert.doesNotThrow(() => {
        for (let i = rs2.camera_info.CAMERA_INFO_NAME; i < rs2.camera_info.CAMERA_INFO_COUNT; i++) {
          let info = rs2.camera_info.cameraInfoToString(i);
          let obj = sensor.supportsCameraInfo(info);
          assert.equal(typeof obj, 'boolean');
        }
      });
    });
  });

  it('Testing method getCameraInfo w/ invaild info', () => {
    sensors.forEach((sensor) => {
      assert.throws(() => {
        sensor.getCameraInfo('dummy');
      });
    });
  });

  it('Testing method supportsCameraInfo w/ invaild info', () => {
    sensors.forEach((sensor) => {
      assert.throws(() => {
        sensor.supportsCameraInfo('dummy');
      });
    });
  });

  it('Testing method supportsCameraInfo w/o args ', () => {
    sensors.forEach((sensor) => {
      assert.throws(() => {
        sensor.supportsCameraInfo();
      });
    });
  });

  it('Testing method getCameraInfo w/o args', () => {
    sensors.forEach((sensor) => {
      assert.throws(() => {
        sensor.getCameraInfo();
      });
    });
  });
});
