// Copyright (c) 2017 Intel Corporation. All rights reserved.
// Use of this source code is governed by an Apache 2.0 license
// that can be found in the LICENSE file.

'use strict';

/* global describe, it, beforeEach, afterEach, after */
const assert = require('assert');
const fs = require('fs');
const path = require('path');
let rs2;
try {
  rs2 = require('node-librealsense');
} catch (e) {
  rs2 = require('../index.js');
}

let ctx;
let dev;
let recorder;
let sensor;
let fileName = 'record.bag';

describe('RecorderDevice constructor test', () => {
  beforeEach(() => {
    ctx = new rs2.Context();
    const devices = ctx.queryDevices().devices;
    assert(devices.length > 0);
    dev = devices[0];
  });

  afterEach(() => {
    rs2.cleanup();
  });

  after(() => {
    if (fs.existsSync(fileName)) {
      fs.unlinkSync(fileName);
    }
  });

  it('Testing constructor - valid argument', () => {
    assert.doesNotThrow(() => {
      new rs2.RecorderDevice(fileName, dev);
    });
  });

  it('Testing constructor - 1 argument', () => {
    assert.throws(() => {
      new rs2.RecorderDevice(fileName);
    });
  });

  it('Testing constructor - invalid file argument', () => {
    assert.throws(() => {
      new rs2.RecorderDevice(1, dev);
    });
  });

  it('Testing constructor - invalid devices argument', () => {
    assert.throws(() => {
      new rs2.RecorderDevice(fileName, 1);
    });
  });

  it('Testing constructor - 5 arguments', () => {
    assert.throws(() => {
      new rs2.RecorderDevice(fileName, dev, undefined, true, true);
    });
  });
});

describe('RecorderDevice test', function() {
  after(function() {
    if (fs.existsSync(fileName)) {
      fs.unlinkSync(fileName);
    }
  });
  afterEach(() => {
    rs2.cleanup();
  });

  function startRecording(file, cnt, callback) {
    return new Promise((resolve, reject) => {
      setTimeout(() => {
        ctx = new rs2.Context();
        dev = ctx.queryDevices().devices[0];
        recorder = new rs2.RecorderDevice(file, dev);
        let sensors = recorder.querySensors();
        sensor = sensors[0];
        let profiles = sensor.getStreamProfiles();
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
          if (callback) {
            callback(recorder, counter);
          }
          counter++;
          if (counter === cnt) {
            sensor.stop();
            recorder.reset();
            rs2.cleanup();
            resolve();
          }
        });
      }, 2000);
    });
  }

  it('Testing method getCameraInfo - without argument', () => {
    return new Promise((resolve, reject) => {
      startRecording(fileName, 1, (recorder, cnt) => {
        assert.doesNotThrow(() => {
          recorder.getCameraInfo();
        });
      }).then(() => {
        resolve();
      });
    });
  }).timeout(5000);

  it('Testing method getCameraInfo - return value', () => {
    return new Promise((resolve, reject) => {
      startRecording(fileName, 1, (recorder, cnt) => {
        let rtn = recorder.getCameraInfo();
        assert.equal(typeof rtn.name, 'string');
        assert.equal(typeof rtn.serialNumber, 'string');
        assert.equal(typeof rtn.firmwareVersion, 'string');
        assert.equal(typeof rtn.physicalPort, 'string');
        assert.equal(typeof rtn.debugOpCode, 'string');
        assert.equal(typeof rtn.productId, 'string');
      }).then(() => {
        resolve();
      });
    });
  }).timeout(5000);

  it('Testing method getCameraInfo - with argument', () => {
    return new Promise((resolve, reject) => {
      startRecording(fileName, 1, (recorder, cnt) => {
        for (let i in rs2.camera_info) {
          if (rs2.camera_info[i] &&
            i.toUpperCase() !== 'CAMERA_INFO_LOCATION' && // location is not ready
            i.toUpperCase() !== 'CAMERA_INFO_COUNT' && // skip counter
            i.toUpperCase() !== 'CAMERA_INFO_CAMERA_LOCKED' &&
            i.toUpperCase() !== 'CAMERA_INFO_ADVANCED_MODE' &&
            i !== 'cameraInfoToString' // skip method
          ) {
            assert.doesNotThrow(() => { // jshint ignore:line
              recorder.getCameraInfo(rs2.camera_info[i]);
            });
          }
        }
      }).then(() => {
        resolve();
      });
    });
  }).timeout(5000);

  it('Testing method getCameraInfo - with invalid argument', () => {
    return new Promise((resolve, reject) => {
      startRecording(fileName, 1, (recorder, cnt) => {
        assert.throws(() => {
          recorder.getCameraInfo('dummy');
        });
      }).then(() => {
        resolve();
      });
    });
  }).timeout(5000);

  it('Testing method querySensors - return value', () => {
    return new Promise((resolve, reject) => {
      startRecording(fileName, 1, (recorder, cnt) => {
        assert.doesNotThrow(() => {
          recorder.querySensors();
        });
        assert.equal(Object.prototype.toString.call(recorder.querySensors()), '[object Array]');
        recorder.querySensors().forEach((i) => {
          assert(recorder.querySensors()[0] instanceof rs2.Sensor);
        });
      }).then(() => {
        resolve();
      });
    });
  }).timeout(5000);

  it('Testing method reset', () => {
    return new Promise((resolve, reject) => {
      startRecording(fileName, 1, (recorder, cnt) => {
        assert.doesNotThrow(() => {
          recorder.reset();
        });
        assert.equal(typeof recorder.reset(), 'undefined');
      }).then(() => {
        resolve();
      });
    });
  }).timeout(5000);

  it('Testing method supportsCameraInfo - without argument', () => {
    return new Promise((resolve, reject) => {
      startRecording(fileName, 1, (recorder, cnt) => {
        assert.throws(() => {
          recorder.supportsCameraInfo();
        });
      }).then(() => {
        resolve();
      });
    });
  }).timeout(5000);

  it('Testing method supportsCameraInfo - return value', () => {
    return new Promise((resolve, reject) => {
      startRecording(fileName, 1, (recorder, cnt) => {
        let rtn = recorder.supportsCameraInfo('serial-number');
        assert.equal(typeof rtn, 'boolean');
      }).then(() => {
        resolve();
      });
    });
  }).timeout(5000);

  it('Testing method supportsCameraInfo - with invalid argument', () => {
    return new Promise((resolve, reject) => {
      startRecording(fileName, 1, (recorder, cnt) => {
        assert.throws(() => {
          recorder.supportsCameraInfo('dummy');
        });
      }).then(() => {
        resolve();
      });
    });
  }).timeout(5000);

  it('Testing method supportsCameraInfo - with argument', () => {
    return new Promise((resolve, reject) => {
      startRecording(fileName, 1, (recorder, cnt) => {
        for (let i in rs2.camera_info) {
          if (rs2.camera_info[i] &&
            i.toUpperCase() !== 'CAMERA_INFO_LOCATION' && // BUG-7429
            i.toUpperCase() !== 'CAMERA_INFO_COUNT' && // skip counter
            i !== 'cameraInfoToString' // skip method
            ) {
            assert.doesNotThrow(() => { // jshint ignore:line
              recorder.supportsCameraInfo(rs2.camera_info[i]);
            });
          }
        }
      }).then(() => {
        resolve();
      });
    });
  }).timeout(5000);

  it('Testing method pause - valid', () => {
    return new Promise((resolve, reject) => {
      startRecording(fileName, 1, (recorder, cnt) => {
        if (cnt === 1) {
          assert.doesNotThrow(() => {
            recorder.pause();
            recorder.resume();
          });
        }
      }).then(() => {
        resolve();
      });
    });
  }).timeout(5000);

  it('Testing method pause - 1 argument', () => {
    return new Promise((resolve, reject) => {
      startRecording(fileName, 1, (recorder, cnt) => {
        if (cnt === 1) {
          assert.doesNotThrow(() => {
            recorder.pause('dummy');
            recorder.resume();
          });
        }
      }).then(() => {
        resolve();
      });
    });
  }).timeout(5000);

  it('Testing method resume - 1 argument', () => {
    return new Promise((resolve, reject) => {
      startRecording(fileName, 1, (recorder, cnt) => {
        if (cnt === 1) {
          assert.doesNotThrow(() => {
            recorder.pause();
            recorder.resume('dummy');
          });
        }
      }).then(() => {
        resolve();
      });
    });
  }).timeout(5000);

  it('Testing method pause resume - more than once', () => {
    return new Promise((resolve, reject) => {
      startRecording(fileName, 1, (recorder, cnt) => {
        if (cnt === 1) {
          assert.doesNotThrow(() => {
            recorder.pause();
            recorder.resume();
            recorder.pause();
            recorder.resume();
            recorder.pause();
            recorder.resume();
          });
        }
      }).then(() => {
        resolve();
      });
    });
  }).timeout(5000);

  it('Testing member - fileName', () => {
    return new Promise((resolve, reject) => {
      startRecording(fileName, 1, (recorder, cnt) => {
        if (cnt === 1) {
          assert(recorder.fileName === fileName);
        }
      }).then(() => {
        assert(fs.existsSync(fileName));
        resolve();
      }).catch((e) => {
        reject(e);
      });
    });
  }).timeout(5000);

  it('Testing member - fileName with absolute path', () => {
    fileName = path.join(__dirname, 'record.bag');
    return new Promise((resolve, reject) => {
      startRecording(fileName, 1, (recorder, cnt) => {
        if (cnt === 1) {
          assert(recorder.fileName === fileName);
          assert(fs.existsSync(fileName));
        }
      }).then(() => {
        resolve();
      }).catch((e) => {
        reject(e);
      });
    });
  }).timeout(5000);

  it('Testing member - fileName is existing', () => {
    return new Promise((resolve, reject) => {
      fs.closeSync(fs.openSync(fileName, 'w'));
      assert(fs.existsSync(fileName));
      startRecording(fileName, 1, (recorder, cnt) => {
        if (cnt === 1) {
          assert(recorder.fileName === fileName);
          assert(fs.existsSync(fileName));
        }
      }).then(() => {
        resolve();
      }).catch((e) => {
        reject(e);
      });
    });
  }).timeout(5000);
});
