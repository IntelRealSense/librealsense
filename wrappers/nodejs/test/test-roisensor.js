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
      sensor.destroy();
    });
    rs2.cleanup();
  });

  it('Testing constructor', () => {
    assert.doesNotThrow(() => {
      new rs2.ROISensor();
    });
  });

  it('Testing method getRegionOfInterest, ', () => {
    sensors.forEach((sensor) => {
      let roi = rs2.ROISensor.from(sensor);
      if (roi) {
        assert.doesNotThrow(() => {
          let res = roi.getRegionOfInterest();
          assert.strictEqual(typeof res, 'object');
          assert.strictEqual(typeof res.minX, 'number');
          assert.strictEqual(typeof res.minY, 'number');
          assert.strictEqual(typeof res.maxX, 'number');
          assert.strictEqual(typeof res.maxY, 'number');
        });
      }
    });
  });

  it('Testing method setRegionOfInterest(region), w/ invalid region - number', () => {
    sensors.forEach((sensor) => {
      let roi = rs2.ROISensor.from(sensor);
      if (roi) {
        let res1 = roi.getRegionOfInterest();
        assert.throws(() => {
          roi.setRegionOfInterest(1);
        });
        let res2 = roi.getRegionOfInterest();
        assert.strictEqual(res1, res2);
      }
    });
  });

  it('Testing method setRegionOfInterest(region), w/ invalid region - string', () => {
    sensors.forEach((sensor) => {
      let roi = rs2.ROISensor.from(sensor);
      if (roi) {
        let res1 = roi.getRegionOfInterest();
        assert.throws(() => {
          roi.setRegionOfInterest('dummy');
        });
        let res2 = roi.getRegionOfInterest();
        assert.strictEqual(res1, res2);
      }
    });
  });

  it('Testing method setRegionOfInterest(region), w/ valid region - origin', () => {
    let ROIobj = {
      minX: 0,
      minY: 0,
      maxX: 0,
      maxY: 0,
    };
    sensors.forEach((sensor) => {
      let roi = rs2.ROISensor.from(sensor);
      if (roi) {
        assert.doesNotThrow(() => {
          roi.setRegionOfInterest(ROIobj);
        });
        assert.doesNotThrow(() => {
          let res = roi.getRegionOfInterest();
          assert.strictEqual(typeof res, 'object');
          assert.strictEqual(res.maxX, ROIobj.maxX);
          assert.strictEqual(res.maxY, ROIobj.maxY);
          assert.strictEqual(res.minX, ROIobj.minX);
          assert.strictEqual(res.minY, ROIobj.minY);
        });
      }
    });
  });

  it('Testing method setRegionOfInterest(region), w/ float region', () => {
    let ROIobj = {
      minX: 1.1,
      minY: 1.1,
      maxX: 9.9,
      maxY: 9.9,
    };
    sensors.forEach((sensor) => {
      let roi = rs2.ROISensor.from(sensor);
      if (roi) {
        assert.doesNotThrow(() => {
          roi.setRegionOfInterest(ROIobj);
        });
        assert.doesNotThrow(() => {
          let res = roi.getRegionOfInterest();
          assert.strictEqual(typeof res, 'object');
          assert.strictEqual(res.maxX, ROIobj.maxX);
          assert.strictEqual(res.maxY, ROIobj.maxY);
          assert.strictEqual(res.minX, ROIobj.minX);
          assert.strictEqual(res.minY, ROIobj.minY);
        });
      }
    });
  });

  it('Testing method setRegionOfInterest(region), w/ min bigger than max value', () => {
    let ROIobj = {
      minX: 10,
      minY: 10,
      maxX: 1,
      maxY: 1,
    };
    sensors.forEach((sensor) => {
      let roi = rs2.ROISensor.from(sensor);
      if (roi) {
        let res1 = roi.getRegionOfInterest();
        assert.throws(() => {
          roi.setRegionOfInterest(ROIobj);
        });
        let res2 = roi.getRegionOfInterest();
        assert.strictEqual(res1, res2);
      }
    });
  });

  it('Testing method setRegionOfInterest(region), w/ x value w/o y value', () => {
    let ROIobj = {
      minX: 5,
      maxX: 5,
    };
    sensors.forEach((sensor) => {
      let roi = rs2.ROISensor.from(sensor);
      if (roi) {
        let res1 = roi.getRegionOfInterest();
        assert.throws(() => {
          roi.setRegionOfInterest(ROIobj);
        });
        let res2 = roi.getRegionOfInterest();
        assert.strictEqual(res1, res2);
      }
    });
  });

  it('Testing method setRegionOfInterest(region), w/ max value w/o min value', () => {
    let ROIobj = {
      maxX: 5,
      maxY: 5,
    };
    sensors.forEach((sensor) => {
      let roi = rs2.ROISensor.from(sensor);
      if (roi) {
        assert.doesNotThrow(() => {
          roi.setRegionOfInterest(ROIobj);
        });
        assert.doesNotThrow(() => {
          let res = roi.getRegionOfInterest();
          assert.equal(typeof res, 'object');
          assert.strictEqual(res.maxX, ROIobj.maxX);
          assert.strictEqual(res.maxy, ROIobj.maxY);
        });
      }
    });
  });

  it('Testing method setRegionOfInterest(region), w/ invalid value of region - maxZ', () => {
    let ROIobj = {
      maxZ: 5,
      maxW: 5,
      minZ: 5,
      minW: 5,
    };
    sensors.forEach((sensor) => {
      let roi = rs2.ROISensor.from(sensor);
      if (roi) {
        let res1 = roi.getRegionOfInterest();
        assert.throws(() => {
          roi.setRegionOfInterest(ROIobj);
        });
        let res2 = roi.getRegionOfInterest();
        assert.strictEqual(res1, res2);
      }
    });
  });

  it('Testing method setRegionOfInterest(), w/ two args', () => {
    sensors.forEach((sensor) => {
      let roi = rs2.ROISensor.from(sensor);
      if (roi) {
        let res1 = roi.getRegionOfInterest();
        assert.throws(() => {
          roi.setRegionOfInterest(1, 2);
        });
        let res2 = roi.getRegionOfInterest();
        assert.strictEqual(res1, res2);
      }
    });
  });

  it('Testing method setRegionOfInterest(), w/ five args', () => {
    sensors.forEach((sensor) => {
      let roi = rs2.ROISensor.from(sensor);
      if (roi) {
        let res1 = roi.getRegionOfInterest();
        assert.throws(() => {
          roi.setRegionOfInterest(1, 2, 3, 4, 5);
        });
        let res2 = roi.getRegionOfInterest();
        assert.strictEqual(res1, res2);
      }
    });
  });

  it('Testing method setRegionOfInterest(), w/o args', () => {
    sensors.forEach((sensor) => {
      let roi = rs2.ROISensor.from(sensor);
      if (roi) {
        let res1 = roi.getRegionOfInterest();
        assert.throws(() => {
          roi.setRegionOfInterest();
        });
        let res2 = roi.getRegionOfInterest();
        assert.strictEqual(res1, res2);
      }
    });
  });

  it('Testing method setRegionOfInterest(mX, mY, MX, MY), w/ minX, minY, maxX, maxY', () => {
    let minX = 5;
    let minY = 5;
    let maxX = 10;
    let maxY = 10;
    sensors.forEach((sensor) => {
      let roi = rs2.ROISensor.from(sensor);
      if (roi) {
        assert.doesNotThrow(() => {
          roi.setRegionOfInterest(minX, minY, maxX, maxY);
        });
        assert.doesNotThrow(() => {
          let res = roi.getRegionOfInterest();
          assert.equal(typeof res, 'object');
          assert.strictEqual(res.minX, minX);
          assert.strictEqual(res.minY, minY);
          assert.strictEqual(res.maxX, maxX);
          assert.strictEqual(res.maxY, maxY);
        });
      }
    });
  });

  it('Testing method setRegionOfInterest(mX, mY, MX, MY), w/ min bigger than max value', () => {
    let minX = 10;
    let minY = 10;
    let maxX = 5;
    let maxY = 5;
    sensors.forEach((sensor) => {
      let roi = rs2.ROISensor.from(sensor);
      if (roi) {
        let res1 = roi.getRegionOfInterest();
        assert.throws(() => {
          roi.setRegionOfInterest(minX, minY, maxX, maxY);
        });
        let res2 = roi.getRegionOfInterest();
        assert.strictEqual(res1, res2);
      }
    });
  });

  it('Testing method setRegionOfInterest(mX, mY, MX, MY), w/ float value', () => {
    let minX = 5.5;
    let minY = 5.5;
    let maxX = 10.5;
    let maxY = 10.5;
    sensors.forEach((sensor) => {
      let roi = rs2.ROISensor.from(sensor);
      if (roi) {
        assert.doesNotThrow(() => {
          roi.setRegionOfInterest(minX, minY, maxX, maxY);
        });
        assert.doesNotThrow(() => {
          let res = roi.getRegionOfInterest();
          assert.equal(typeof res, 'object');
          assert.strictEqual(res.minX, minX);
          assert.strictEqual(res.minY, minY);
          assert.strictEqual(res.maxX, maxX);
          assert.strictEqual(res.maxY, maxY);
        });
      }
    });
  });

  it('Testing method setRegionOfInterest(mX, mY, MX, MY), w/ origin', () => {
    let minX = 0;
    let minY = 0;
    let maxX = 0;
    let maxY = 0;
    sensors.forEach((sensor) => {
      let roi = rs2.ROISensor.from(sensor);
      if (roi) {
        assert.doesNotThrow(() => {
          roi.setRegionOfInterest(minX, minY, maxX, maxY);
        });
        assert.doesNotThrow(() => {
          let res = roi.getRegionOfInterest();
          assert.equal(typeof res, 'object');
          assert.strictEqual(res.minX, minX);
          assert.strictEqual(res.minY, minY);
          assert.strictEqual(res.maxX, maxX);
          assert.strictEqual(res.maxY, maxY);
        });
      }
    });
  });

  it('Testing method setRegionOfInterest(mX, mY, MX, MY), w/ negative', () => {
    let minX = -1;
    let minY = -1;
    let maxX = -1;
    let maxY = -1;
    sensors.forEach((sensor) => {
      let roi = rs2.ROISensor.from(sensor);
      if (roi) {
        let res1 = roi.getRegionOfInterest();
        assert.throws(() => {
          roi.setRegionOfInterest(minX, minY, maxX, maxY);
        });
        let res2 = roi.getRegionOfInterest();
        assert.strictEqual(res1, res2);
      }
    });
  });

  it('Testing method setRegionOfInterest(mX, mY, MX, MY), w/ negative and positive number', () => {
    let minX = -1;
    let minY = -1;
    let maxX = 0;
    let maxY = 0;
    sensors.forEach((sensor) => {
      let roi = rs2.ROISensor.from(sensor);
      if (roi) {
        let res1 = roi.getRegionOfInterest();
        assert.throws(() => {
          roi.setRegionOfInterest(minX, minY, maxX, maxY);
        });
        let res2 = roi.getRegionOfInterest();
        assert.strictEqual(res1, res2);
      }
    });
  });

  it('Testing method setRegionOfInterest(mX, mY, MX, MY), w/ string', () => {
    let minX = 'dummy';
    let minY = 'dummy';
    let maxX = 'dummy';
    let maxY = 'dummy';
    sensors.forEach((sensor) => {
      let roi = rs2.ROISensor.from(sensor);
      if (roi) {
        let res1 = roi.getRegionOfInterest();
        assert.throws(() => {
          roi.setRegionOfInterest(minX, minY, maxX, maxY);
        });
        let res2 = roi.getRegionOfInterest();
        assert.strictEqual(res1, res2);
      }
    });
  });
});
