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

let fileName = 'record.bag';
let config;
let pipeline;
let serial;
describe('Config test', function() {
  beforeEach(function() {
    pipeline = new rs2.Pipeline();
    config = new rs2.Config();
    const ctx = new rs2.Context();
    const devices = ctx.queryDevices().devices;
    assert(devices.length > 0); // Device must be connected
    const dev = devices[0];
    serial = dev.getCameraInfo(rs2.camera_info.CAMERA_INFO_SERIAL_NUMBER);
  });

  afterEach(function() {
    pipeline.destroy();
    rs2.cleanup();
    if (fs.existsSync(fileName)) {
      fs.unlinkSync(fileName);
    }
  });

  function startRecording(file, cnt) {
    return new Promise((resolve, reject) => {
      setTimeout(() => {
        let ctx = new rs2.Context();
        let dev = ctx.queryDevices().devices[0];
        let recorder = new rs2.RecorderDevice(file, dev);
        let sensors = recorder.querySensors();
        let sensor = sensors[0];
        let profiles = sensor.getStreamProfiles();
        assert.equal(recorder.fileName, file);
        for (let i = 0; i < profiles.length; i++) {
          if (profiles[i].streamType === rs2.stream.STREAM_DEPTH &&
              profiles[i].fps === 30 &&
              profiles[i].width === 640 &&
              profiles[i].height === 480 &&
              profiles[i].format === rs2.format.FORMAT_Z16) {
            sensor.open(profiles[i]);
          }
        }
        let counter = 0;
        sensor.start((frame) => {
          counter++;
          if (counter === cnt) {
            recorder.reset();
            resolve();
          }
        });
      }, 2000);
    });
  }

  it('Testing method canResolve - 0 argument', () => {
    assert.throws(() => {
      config.canResolve();
    });
  });

  it('Testing method canResolve - 2 argument', () => {
    assert.throws(() => {
      config.canResolve(pipeline, pipeline);
    });
  });

  it('Testing method canResolve - invalid argument', () => {
    assert.throws(() => {
      config.canResolve('dummy');
    });
  });

  it('Testing method canResolve - valid argument', () => {
    let rtn;
    assert.doesNotThrow(() => {
      rtn = config.canResolve(pipeline);
    });
    assert.equal(typeof rtn, 'boolean');
  });

  it('Testing method disableAllStreams - 0 argument', () => {
    assert.doesNotThrow(() => {
      config.disableAllStreams();
    });
  });

  it('Testing method disableAllStreams - 1 argument', () => {
    assert.doesNotThrow(() => {
      config.disableAllStreams(1);
    });
  });

  it('Testing method disableStream - 0 argument', () => {
    assert.throws(() => {
      config.disableStream();
    });
  });

  it('Testing method disableStream - invalid argument', () => {
    assert.throws(() => {
      config.disableStream('dummy');
    });
  });

  it('Testing method disableStream - valid argument', () => {
    for (let i in rs2.stream) {
      if (rs2.stream[i] &&
        i.toUpperCase() !== 'STREAM_COUNT' && // skip counter
        i !== 'streamToString' // skip method
      ) {
        assert.doesNotThrow(() => { // jshint ignore:line
          config.disableStream(rs2.stream[i]);
        });
      }
    }
  });

  it('Testing method enableDevice - 0 argument', () => {
    assert.throws(() => {
      config.enableDevice();
    });
  });

  it('Testing method enableDevice - invalid argument', () => {
    assert.doesNotThrow(() => {
      config.enableDevice('dummy');
    });
  });

  it('Testing method enableDevice - valid argument', () => {
    assert.doesNotThrow(() => {
      config.enableDevice(serial);
    });
  });

  it('Testing method enableStream - 0 argument', () => {
    assert.throws(() => {
      config.enableStream();
    });
  });

  it('Testing method enableStream - invalid argument', () => {
    assert.throws(() => {
      config.enableStream('dummy', 1, 1, 1, 1, 1);
    });
  });

  it('Testing method enableStream - valid argument', () => {
    // eslint-disable-next-line guard-for-in
    for (let i in rs2.stream) {
      for (let j in rs2.format) {
        if (rs2.stream[i] &&
          i.toUpperCase() !== 'STREAM_COUNT' && // skip counter
          i !== 'streamToString' && // skip method
          rs2.format[j] &&
          j.toUpperCase() !== 'FORMAT_COUNT' && // skip counter
          j !== 'formatToString' // skip method
        ) {
          assert.doesNotThrow(() => { // jshint ignore:line
            config.enableStream(rs2.stream[i], 1, 1, 1, rs2.format[j], 1);
          });
        }
      }
    }
  });

  it('Testing method resolve - 0 argument', () => {
    assert.throws(() => {
      config.resolve();
    });
  });

  it('Testing method resolve - invalid argument', () => {
    assert.throws(() => {
      config.resolve('dummy');
    });
  });

  it('Testing method resolve - valid argument', () => {
    let pp;
    assert.doesNotThrow(() => {
      pp = config.resolve(pipeline);
    });
    assert(pp instanceof rs2.PipelineProfile);
  });

  it('Testing method enableAllStreams - 0 argument', () => {
    assert.doesNotThrow(() => {
      config.enableAllStreams();
    });
  });

  it('Testing method enableAllStreams - 1 argument', () => {
    assert.doesNotThrow(() => {
      config.enableAllStreams(1);
    });
  });

  it('Testing method enableDeviceFromFile - 0 argument', () => {
    assert.throws(() => {
      config.enableDeviceFromFile();
    });
  });

  it('Testing method enableDeviceFromFile - 2 argument', () => {
    assert.throws(() => {
      config.enableDeviceFromFile(1, 1);
    });
  });

  it('Testing method enableDeviceFromFile - argument with number', () => {
    assert.throws(() => {
      config.enableDeviceFromFile(1);
    });
  });

  it('Testing method enableDeviceFromFile - with file', () => {
    return new Promise((resolve, reject) => {
      startRecording(fileName, 10).then(() => {
        assert.doesNotThrow(() => {
          config.enableDeviceFromFile(fileName);
        });
        resolve();
      });
    });
  }).timeout(5000);

  it('Testing method enableDeviceFromFile - with invalid argument', () => {
    assert.throws(() => {
      config.enableDeviceFromFile('dummy');
    });
  });

  it('Testing method enableDeviceFromFile - after enableRecordToFile', () => {
    assert.doesNotThrow(() => {
      config.enableRecordToFile(fileName);
      pipeline.start(config);
    });
    assert.throws(() => {
      config.enableDeviceFromFile(fileName);
    });
  });

  it('Testing method enableRecordToFile - 0 argument', () => {
    assert.throws(() => {
      config.enableRecordToFile();
    });
  });

  it('Testing method enableRecordToFile - two arguments', () => {
    assert.throws(() => {
      config.enableRecordToFile(fileName, fileName);
    });
  });

  it('Testing method enableRecordToFile - argument with number', () => {
    assert.throws(() => {
      config.enableRecordToFile(1);
    });
  });

  it('Testing method enableRecordToFile - without pipeline.start', () => {
    assert.doesNotThrow(() => {
      config.enableRecordToFile(fileName);
    });
    assert.equal(fs.existsSync(fileName), false);
  });

  it('Testing method enableRecordToFile - value result', () => {
    assert.doesNotThrow(() => {
      config.enableRecordToFile(fileName);
      pipeline.start(config);
    });
    assert.equal(fs.existsSync(fileName), true);
    assert.doesNotThrow(() => {
      let frameSet = pipeline.waitForFrames();
      assert(frameSet instanceof rs2.FrameSet);
    });
  }).timeout(10000);

  it('Testing method enableRecordToFile - then enableDeviceFromFile', () => {
    assert.doesNotThrow(() => {
      config.enableRecordToFile(fileName);
      pipeline.start(config);
      let frameSet = pipeline.waitForFrames();
      assert(frameSet instanceof rs2.FrameSet);
    });
    assert.equal(fs.existsSync(fileName), true);
    assert.throws(() => {
      config.enableDeviceFromFile(fileName);
    });
  });

  it('Testing method enableRecordToFile - fileName has been exist', () => {
    fs.closeSync(fs.openSync(fileName, 'w'));
    assert.doesNotThrow(() => {
      config.enableRecordToFile(fileName);
      pipeline.start(config);
    });
    assert.doesNotThrow(() => {
      let frameSet = pipeline.waitForFrames();
      assert(frameSet instanceof rs2.FrameSet);
    });
    assert.equal(fs.existsSync(fileName), true);
  });
});
