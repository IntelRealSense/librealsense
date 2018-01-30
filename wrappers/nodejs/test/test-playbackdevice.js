// Copyright (c) 2017 Intel Corporation. All rights reserved.
// Use of this source code is governed by an Apache 2.0 license
// that can be found in the LICENSE file.

'use strict';

/* global describe, it, before, afterEach, after */
const assert = require('assert');
const fs = require('fs');
let rs2;
try {
  rs2 = require('node-librealsense');
} catch (e) {
  rs2 = require('../index.js');
}

let fileName = 'record.bag';
describe('PlayBackDevice test', function() {
  before(function(done) {
    /* eslint-disable no-invalid-this */
    this.timeout(5000);
    const ctx = new rs2.Context();
    const devices = ctx.queryDevices().devices;
    assert(devices.length > 0); // Device must be connected
    startRecording(fileName, 3, null).then(() => {
      done();
    });
  });

  afterEach(function() {
    rs2.cleanup();
  });

  after(function() {
    if (fs.existsSync(fileName)) {
      fs.unlinkSync(fileName);
    }
  });

  function startRecording(file, cnt, callback) {
    return new Promise((resolve, reject) => {
      setTimeout(() => {
        let ctx = new rs2.Context();
        let dev = ctx.queryDevices().devices[0];
        let recorder = new rs2.RecorderDevice(file, dev);
        let sensors = recorder.querySensors();
        let sensor = sensors[0];
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
            recorder.reset();
            rs2.cleanup();
            resolve();
          }
        });
      }, 2000);
    });
  }

  function startPlayback(file, callback) {
    let runCB = true;
    return new Promise((resolve, reject) => {
      let ctx = new rs2.Context();
      let dev = ctx.loadDevice(file);
      let sensors = dev.querySensors();
      let sensor = sensors[0];
      let profiles = sensor.getStreamProfiles();
      let cnt = 0;

      dev.setStatusChangedCallback((status) => {
        if (runCB) {
          runCB = false;
          callback(dev, status, cnt);
        }
        if (status.description === 'stopped') {
          dev.stop();
          dev.destroy();
          ctx.unloadDevice(file);
          rs2.cleanup();
          resolve();
        }
      });
      sensor.open(profiles);
      sensor.start((frame) => {
        cnt++;
      });
    });
  }

  it('Testing constructor - valid argument', () => {
    let ctx = new rs2.Context();
    assert.doesNotThrow(() => {
      ctx.loadDevice(fileName);
    });
  });

  it('Testing constructor - 0 argument', () => {
    let ctx = new rs2.Context();
    assert.throws(() => {
      ctx.loadDevice();
    });
  });

  it('Testing constructor - 2 arguments', () => {
    let ctx = new rs2.Context();
    assert.throws(() => {
      ctx.loadDevice(fileName, 1);
    });
  });

  it('Testing constructor - invalid file argument', () => {
    let ctx = new rs2.Context();
    assert.throws(() => {
      ctx.loadDevice(1);
    });
  });

  it('Testing constructor - invalid file content', () => {
    let ctx = new rs2.Context();
    assert.throws(() => {
      ctx.loadDevice('./test-playbackdevice.js');
    });
  });

  it('Testing member currentStatus', () => {
    return new Promise((resolve, reject) => {
      startPlayback(fileName, (playbackDev, status) => {
        assert.equal(typeof playbackDev.currentStatus.status, 'number');
        assert.equal(playbackDev.currentStatus.status, status.status);
        assert.equal(typeof playbackDev.currentStatus.description, 'string');
        assert.equal(playbackDev.currentStatus.description, status.description);
      }).then(() => {
        resolve();
      });
    });
  }).timeout(5000);

  it('Testing member duration', () => {
    return new Promise((resolve, reject) => {
      startPlayback(fileName, (playbackDev) => {
        assert.equal(typeof playbackDev.duration, 'number');
      }).then(() => {
        resolve();
      });
    });
  }).timeout(5000);

  it('Testing member fileName', () => {
    return new Promise((resolve, reject) => {
      startPlayback(fileName, (playbackDev) => {
        assert.equal(typeof playbackDev.fileName, 'string');
        assert.equal(playbackDev.fileName, fileName);
      }).then(() => {
        resolve();
      });
    });
  }).timeout(5000);

  it('Testing member first', () => {
    return new Promise((resolve, reject) => {
      startPlayback(fileName, (playbackDev) => {
        assert.equal(typeof playbackDev.first, 'object');
        assert(playbackDev.first instanceof rs2.Sensor);
      }).then(() => {
        resolve();
      });
    });
  }).timeout(5000);

  it('Testing member isRealTime', () => {
    return new Promise((resolve, reject) => {
      startPlayback(fileName, (playbackDev) => {
        assert.equal(typeof playbackDev.isRealTime, 'boolean');
      }).then(() => {
        resolve();
      });
    });
  }).timeout(5000);

  it('Testing member isValid', () => {
    return new Promise((resolve, reject) => {
      startPlayback(fileName, (playbackDev) => {
        assert.equal(typeof playbackDev.isValid, 'boolean');
      }).then(() => {
        resolve();
      });
    });
  }).timeout(5000);

  it('Testing member position', () => {
    return new Promise((resolve, reject) => {
      startPlayback(fileName, (playbackDev) => {
        assert.equal(typeof playbackDev.position, 'number');
      }).then(() => {
        resolve();
      });
    });
  }).timeout(5000);

  it('Testing method getCameraInfo - without argument', () => {
    return new Promise((resolve, reject) => {
      startPlayback(fileName, (playbackDev) => {
        assert.doesNotThrow(() => {
          playbackDev.getCameraInfo();
        });
      }).then(() => {
        resolve();
      });
    });
  }).timeout(5000);

  it('Testing method getCameraInfo - return value', () => {
    return new Promise((resolve, reject) => {
      startPlayback(fileName, (playbackDev) => {
        let rtn = playbackDev.getCameraInfo();
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
      startPlayback(fileName, (playbackDev) => {
        for (let i in rs2.camera_info) {
          if (rs2.camera_info[i] &&
            i.toUpperCase() !== 'CAMERA_INFO_LOCATION' && // location is not ready
            i.toUpperCase() !== 'CAMERA_INFO_COUNT' && // skip counter
            i.toUpperCase() !== 'CAMERA_INFO_CAMERA_LOCKED' &&
            i.toUpperCase() !== 'CAMERA_INFO_ADVANCED_MODE' &&
            i !== 'cameraInfoToString' // skip method
          ) {
            assert.doesNotThrow(() => { // jshint ignore:line
              playbackDev.getCameraInfo(rs2.camera_info[i]);
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
      startPlayback(fileName, (playbackDev) => {
        assert.throws(() => {
          playbackDev.getCameraInfo('dummy');
        });
      }).then(() => {
        resolve();
      });
    });
  }).timeout(5000);

  it('Testing method querySensors - return value', () => {
    return new Promise((resolve, reject) => {
      startPlayback(fileName, (playbackDev) => {
        assert.doesNotThrow(() => {
          playbackDev.querySensors();
        });
        assert.equal(Object.prototype.toString.call(playbackDev.querySensors()), '[object Array]');
        playbackDev.querySensors().forEach((i) => {
          assert(playbackDev.querySensors()[0] instanceof rs2.Sensor);
        });
      }).then(() => {
        resolve();
      });
    });
  }).timeout(5000);

  it('Testing method reset', () => {
    return new Promise((resolve, reject) => {
      startPlayback(fileName, (playbackDev) => {
        assert.doesNotThrow(() => {
          playbackDev.reset();
        });
        assert.equal(typeof playbackDev.reset(), 'undefined');
      }).then(() => {
        resolve();
      });
    });
  }).timeout(5000);

  it('Testing method supportsCameraInfo - without argument', () => {
    return new Promise((resolve, reject) => {
      startPlayback(fileName, (playbackDev) => {
        assert.throws(() => {
          playbackDev.supportsCameraInfo();
        });
      }).then(() => {
        resolve();
      });
    });
  }).timeout(5000);

  it('Testing method supportsCameraInfo - return value', () => {
    return new Promise((resolve, reject) => {
      startPlayback(fileName, (playbackDev) => {
        let rtn = playbackDev.supportsCameraInfo('serial-number');
        assert.equal(typeof rtn, 'boolean');
      }).then(() => {
        resolve();
      });
    });
  }).timeout(5000);

  it('Testing method supportsCameraInfo - with invalid argument', () => {
    return new Promise((resolve, reject) => {
      startPlayback(fileName, (playbackDev) => {
        assert.throws(() => {
          playbackDev.supportsCameraInfo('dummy');
        });
      }).then(() => {
        resolve();
      });
    });
  }).timeout(5000);

  it('Testing method supportsCameraInfo - with argument', () => {
    return new Promise((resolve, reject) => {
      startPlayback(fileName, (playbackDev) => {
        for (let i in rs2.camera_info) {
          if (rs2.camera_info[i] &&
            i.toUpperCase() !== 'CAMERA_INFO_LOCATION' && // BUG-7429
            i.toUpperCase() !== 'CAMERA_INFO_COUNT' && // skip counter
            i !== 'cameraInfoToString' // skip method
            ) {
            assert.doesNotThrow(() => { // jshint ignore:line
              playbackDev.supportsCameraInfo(rs2.camera_info[i]);
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
      startPlayback(fileName, (playbackDev, status, cnt) => {
        assert.doesNotThrow(() => {
          playbackDev.pause();
          playbackDev.resume();
          playbackDev.stop();
        });
      }).then(() => {
        resolve();
      });
    });
  }).timeout(5000);

  it('Testing method resume - 1 argument', () => {
    return new Promise((resolve, reject) => {
      startPlayback(fileName, (playbackDev, status, cnt) => {
        assert.doesNotThrow(() => {
          playbackDev.pause();
          playbackDev.resume('dummy');
        });
      }).then(() => {
        resolve();
      });
    });
  }).timeout(5000);

  it('Testing method querySensors - valid argument', () => {
    return new Promise((resolve, reject) => {
      startPlayback(fileName, (playbackDev, status, cnt) => {
        assert.doesNotThrow(() => {
          playbackDev.querySensors();
        });
      }).then(() => {
        resolve();
      });
    });
  }).timeout(5000);

  it('Testing method querySensors - invalid argument', () => {
    return new Promise((resolve, reject) => {
      startPlayback(fileName, (playbackDev, status, cnt) => {
        assert.doesNotThrow(() => {
          playbackDev.querySensors('dummy');
        });
      }).then(() => {
        resolve();
      });
    });
  }).timeout(5000);

  it('Testing method reset - valid argument', () => {
    return new Promise((resolve, reject) => {
      startPlayback(fileName, (playbackDev, status, cnt) => {
        assert.doesNotThrow(() => {
          playbackDev.reset();
        });
      }).then(() => {
        resolve();
      });
    });
  }).timeout(5000);

  it('Testing method reset - invalid argument', () => {
    return new Promise((resolve, reject) => {
      startPlayback(fileName, (playbackDev, status, cnt) => {
        assert.doesNotThrow(() => {
          playbackDev.reset('dummy');
        });
      }).then(() => {
        resolve();
      });
    });
  }).timeout(5000);

  it('Testing method seek - valid argument', () => {
    return new Promise((resolve, reject) => {
      startPlayback(fileName, (playbackDev, status, cnt) => {
        assert.doesNotThrow(() => {
          playbackDev.seek(1);
        });
      }).then(() => {
        resolve();
      });
    });
  }).timeout(5000);

  it('Testing method seek - invalid argument', () => {
    return new Promise((resolve, reject) => {
      startPlayback(fileName, (playbackDev, status, cnt) => {
        assert.throws(() => {
          playbackDev.seek('dummy');
        });
      }).then(() => {
        resolve();
      });
    });
  }).timeout(5000);

  it('Testing method setPlaybackSpeed - valid argument', () => {
    return new Promise((resolve, reject) => {
      startPlayback(fileName, (playbackDev, status, cnt) => {
        assert.doesNotThrow(() => {
          playbackDev.setPlaybackSpeed(1);
        });
      }).then(() => {
        resolve();
      });
    });
  }).timeout(5000);

  it('Testing method setPlaybackSpeed - invalid argument', () => {
    return new Promise((resolve, reject) => {
      startPlayback(fileName, (playbackDev, status, cnt) => {
        assert.throws(() => {
          playbackDev.setPlaybackSpeed('dummy');
        });
      }).then(() => {
        resolve();
      });
    });
  }).timeout(5000);
});
