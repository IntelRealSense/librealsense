// Copyright (c) 2018 Intel Corporation. All rights reserved.
// Use of this source code is governed by an Apache 2.0 license
// that can be found in the LICENSE file.

'use strict';

/* global describe, it, before, after */
const assert = require('assert');
const fs = require('fs');
let rs2;
try {
  rs2 = require('node-librealsense');
} catch (e) {
  rs2 = require('../index.js');
}

describe('Disparity transform tests', function() {
  let ctx;
  let pipe;
  let depth2Disparity;
  let disparith2Depth;
  before(function() {
    ctx = new rs2.Context();
    pipe = new rs2.Pipeline(ctx);
    depth2Disparity = new rs2.DepthToDisparityTransform();
    disparith2Depth = new rs2.DisparityToDepthTransform();
  });

  after(function() {
    rs2.cleanup();
  });

  it('depth frame <=> disparity frame using transform', () => {
    pipe.start();
    const frames = pipe.waitForFrames();
    assert.equal(frames.size > 0, true);
    frames.forEach((frame) => {
      if (frame instanceof rs2.DepthFrame) {
        let output = depth2Disparity.process(frame);
        assert.equal(output instanceof rs2.DisparityFrame, true);
        assert.equal(typeof output.baseLine, 'number');

        let dout = disparith2Depth.process(output);
        assert.equal(dout instanceof rs2.DepthFrame, true);
      }
    });
    pipe.stop();
  });
});

describe('Points.exportToPly', function() {
  it('exportToPly', () => {
    const pc = new rs2.PointCloud();
    const pipe = new rs2.Pipeline();
    const file = 'points.ply';
    pipe.start();
    const frameset = pipe.waitForFrames();
    const points = pc.calculate(frameset.depthFrame);
    points.exportToPly(file, frameset.colorFrame);
    assert.equal(fs.existsSync(file), true);
    fs.unlinkSync(file);
    pipe.stop();
    rs2.cleanup();
  });
});

describe('enum value test', function() {
  it('sr300_visual_preset test', () => {
    const obj = rs2.sr300_visual_preset;
    const numberAttrs = [
      'SR300_VISUAL_PRESET_SHORT_RANGE',
      'SR300_VISUAL_PRESET_LONG_RANGE',
      'SR300_VISUAL_PRESET_BACKGROUND_SEGMENTATION',
      'SR300_VISUAL_PRESET_GESTURE_RECOGNITION',
      'SR300_VISUAL_PRESET_OBJECT_SCANNING',
      'SR300_VISUAL_PRESET_FACE_ANALYTICS',
      'SR300_VISUAL_PRESET_FACE_LOGIN',
      'SR300_VISUAL_PRESET_GR_CURSOR',
      'SR300_VISUAL_PRESET_DEFAULT',
      'SR300_VISUAL_PRESET_MID_RANGE',
      'SR300_VISUAL_PRESET_IR_ONLY',
      'SR300_VISUAL_PRESET_COUNT',
    ];
    const strAttrs = [
      'sr300_visual_preset_short_range',
      'sr300_visual_preset_long_range',
      'sr300_visual_preset_background_segmentation',
      'sr300_visual_preset_gesture_recognition',
      'sr300_visual_preset_object_scanning',
      'sr300_visual_preset_face_analytics',
      'sr300_visual_preset_face_login',
      'sr300_visual_preset_gr_cursor',
      'sr300_visual_preset_default',
      'sr300_visual_preset_mid_range',
      'sr300_visual_preset_ir_only',
    ];
    numberAttrs.forEach((attr) => {
      assert.equal(typeof obj[attr], 'number');
    });
    strAttrs.forEach((attr) => {
      assert.equal(typeof obj[attr], 'string');
    });
    for (let i = 0; i < obj.SR300_VISUAL_PRESET_COUNT; i++) {
      assert.equal(typeof obj.sr300VisualPresetToString(i), 'string');
    }
  });

  it('rs400_visual_preset test', () => {
    const obj = rs2.rs400_visual_preset;
    const numberAttrs = [
      'RS400_VISUAL_PRESET_CUSTOM',
      'RS400_VISUAL_PRESET_DEFAULT',
      'RS400_VISUAL_PRESET_HAND',
      'RS400_VISUAL_PRESET_HIGH_ACCURACY',
      'RS400_VISUAL_PRESET_HIGH_DENSITY',
      'RS400_VISUAL_PRESET_MEDIUM_DENSITY',
      'RS400_VISUAL_PRESET_COUNT',
    ];
    const strAttrs = [
      'rs400_visual_preset_custom',
      'rs400_visual_preset_default',
      'rs400_visual_preset_hand',
      'rs400_visual_preset_high_accuracy',
      'rs400_visual_preset_high_density',
      'rs400_visual_preset_medium_density',
    ];
    numberAttrs.forEach((attr) => {
      assert.equal(typeof obj[attr], 'number');
    });
    strAttrs.forEach((attr) => {
      assert.equal(typeof obj[attr], 'string');
    });
    for (let i = 0; i < obj.RS400_VISUAL_PRESET_COUNT; i++) {
      assert.equal(typeof obj.rs400VisualPresetToString(i), 'string');
    }
  });

  it('format test', () => {
    const obj = rs2.format;
    const numberAttrs = [
      'FORMAT_ANY',
      'FORMAT_Z16',
      'FORMAT_DISPARITY16',
      'FORMAT_XYZ32F',
      'FORMAT_YUYV',
      'FORMAT_RGB8',
      'FORMAT_BGR8',
      'FORMAT_RGBA8',
      'FORMAT_BGRA8',
      'FORMAT_Y8',
      'FORMAT_Y16',
      'FORMAT_RAW10',
      'FORMAT_RAW16',
      'FORMAT_RAW8',
      'FORMAT_UYVY',
      'FORMAT_MOTION_RAW',
      'FORMAT_MOTION_XYZ32F',
      'FORMAT_GPIO_RAW',
      'FORMAT_6DOF',
      'FORMAT_DISPARITY32',
      'FORMAT_COUNT',
    ];
    const strAttrs = [
      'format_any',
      'format_z16',
      'format_disparity16',
      'format_xyz32f',
      'format_yuyv',
      'format_rgb8',
      'format_bgr8',
      'format_rgba8',
      'format_bgra8',
      'format_y8',
      'format_y16',
      'format_raw10',
      'format_raw16',
      'format_raw8',
      'format_uyvy',
      'format_motion_raw',
      'format_motion_xyz32f',
      'format_gpio_raw',
      'format_6dof',
      'format_disparity32',
    ];
    numberAttrs.forEach((attr) => {
      assert.equal(typeof obj[attr], 'number');
    });
    strAttrs.forEach((attr) => {
      assert.equal(typeof obj[attr], 'string');
    });
    for (let i = 0; i < obj.FORMAT_COUNT; i++) {
      assert.equal(typeof obj.formatToString(i), 'string');
    }
  });
  it('notification_category test', () => {
    const obj = rs2.notification_category;
    const numberAttrs = [
      'NOTIFICATION_CATEGORY_FRAMES_TIMEOUT',
      'NOTIFICATION_CATEGORY_FRAME_CORRUPTED',
      'NOTIFICATION_CATEGORY_HARDWARE_ERROR',
      'NOTIFICATION_CATEGORY_HARDWARE_EVENT',
      'NOTIFICATION_CATEGORY_UNKNOWN_ERROR',
    ];
    const strAttrs = [
      'notification_category_frames_timeout',
      'notification_category_frame_corrupted',
      'notification_category_hardware_error',
      'notification_category_hardware_event',
      'notification_category_unknown_error',
    ];
    numberAttrs.forEach((attr) => {
      assert.equal(typeof obj[attr], 'number');
    });
    strAttrs.forEach((attr) => {
      assert.equal(typeof obj[attr], 'string');
    });
    for (let i = 0; i < obj.NOTIFICATION_CATEGORY_COUNT; i++) {
      assert.equal(typeof obj.notificationCategoryToString(i), 'string');
    }
  });
  it('frame_metadata test', () => {
    const obj = rs2.frame_metadata;
    const numberAttrs = [
      'FRAME_METADATA_FRAME_COUNTER',
      'FRAME_METADATA_FRAME_TIMESTAMP',
      'FRAME_METADATA_SENSOR_TIMESTAMP',
      'FRAME_METADATA_ACTUAL_EXPOSURE',
      'FRAME_METADATA_GAIN_LEVEL',
      'FRAME_METADATA_AUTO_EXPOSURE',
      'FRAME_METADATA_WHITE_BALANCE',
      'FRAME_METADATA_TIME_OF_ARRIVAL',
      'FRAME_METADATA_TEMPERATURE',
    ];
    const strAttrs = [
      'frame_metadata_frame_counter',
      'frame_metadata_frame_timestamp',
      'frame_metadata_sensor_timestamp',
      'frame_metadata_actual_exposure',
      'frame_metadata_gain_level',
      'frame_metadata_auto_exposure',
      'frame_metadata_white_balance',
      'frame_metadata_time_of_arrival',
      'frame_metadata_temperature',
    ];
    numberAttrs.forEach((attr) => {
      assert.equal(typeof obj[attr], 'number');
    });
    strAttrs.forEach((attr) => {
      assert.equal(typeof obj[attr], 'string');
    });
    for (let i = 0; i < obj.FRAME_METADATA_COUNT; i++) {
      assert.equal(typeof obj.frameMetadataToString(i), 'string');
    }
  });
});
