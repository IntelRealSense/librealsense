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

describe('Points and Pointcloud test', function() {
  it('Points.exportToPly', () => {
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
  it('Pointcloud options API test', () => {
    const pc = new rs2.PointCloud();
    for (let i = rs2.option.OPTION_BACKLIGHT_COMPENSATION; i < rs2.option.OPTION_COUNT; i++) {
      if (pc.supportsOption(i)) {
        assert.equal(typeof pc.getOption(i), 'number');
      }
    }
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
      'RS400_VISUAL_PRESET_REMOVE_IR_PATTERN',
      'RS400_VISUAL_PRESET_COUNT',
    ];
    const strAttrs = [
      'rs400_visual_preset_custom',
      'rs400_visual_preset_default',
      'rs400_visual_preset_hand',
      'rs400_visual_preset_high_accuracy',
      'rs400_visual_preset_high_density',
      'rs400_visual_preset_medium_density',
      'rs400_visual_preset_remove_ir_pattern',
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
      'NOTIFICATION_CATEGORY_FIRMWARE_UPDATE_RECOMMENDED',
    ];
    const strAttrs = [
      'notification_category_frames_timeout',
      'notification_category_frame_corrupted',
      'notification_category_hardware_error',
      'notification_category_hardware_event',
      'notification_category_unknown_error',
      'notification_category_firmware_update_recommended',
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
      'FRAME_METADATA_FRAME_LASER_POWER',
      'FRAME_METADATA_FRAME_LASER_POWER_MODE',
      'FRAME_METADATA_EXPOSURE_PRIORITY',
      'FRAME_METADATA_EXPOSURE_ROI_LEFT',
      'FRAME_METADATA_EXPOSURE_ROI_RIGHT',
      'FRAME_METADATA_EXPOSURE_ROI_TOP',
      'FRAME_METADATA_EXPOSURE_ROI_BOTTOM',
      'FRAME_METADATA_BRIGHTNESS',
      'FRAME_METADATA_CONTRAST',
      'FRAME_METADATA_SATURATION',
      'FRAME_METADATA_SHARPNESS',
      'FRAME_METADATA_AUTO_WHITE_BALANCE_TEMPERATURE',
      'FRAME_METADATA_BACKLIGHT_COMPENSATION',
      'FRAME_METADATA_HUE',
      'FRAME_METADATA_GAMMA',
      'FRAME_METADATA_MANUAL_WHITE_BALANCE',
      'FRAME_METADATA_POWER_LINE_FREQUENCY',
      'FRAME_METADATA_LOW_LIGHT_COMPENSATION',
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
      'frame_metadata_frame_laser_power',
      'frame_metadata_frame_laser_power_mode',
      'frame_metadata_exposure_priority',
      'frame_metadata_exposure_roi_left',
      'frame_metadata_exposure_roi_right',
      'frame_metadata_exposure_roi_top',
      'frame_metadata_exposure_roi_bottom',
      'frame_metadata_brightness',
      'frame_metadata_contrast',
      'frame_metadata_saturation',
      'frame_metadata_sharpness',
      'frame_metadata_auto_white_balance_temperature',
      'frame_metadata_backlight_compensation',
      'frame_metadata_hue',
      'frame_metadata_gamma',
      'frame_metadata_manual_white_balance',
      'frame_metadata_power_line_frequency',
      'frame_metadata_low_light_compensation',
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
  it('stream test', () => {
    const obj = rs2.stream;
    const numberAttrs = [
      'STREAM_ANY',
      'STREAM_DEPTH',
      'STREAM_COLOR',
      'STREAM_INFRARED',
      'STREAM_FISHEYE',
      'STREAM_GYRO',
      'STREAM_ACCEL',
      'STREAM_GPIO',
      'STREAM_POSE',
      'STREAM_CONFIDENCE',
    ];
    const strAttrs = [
      'stream_any',
      'stream_depth',
      'stream_color',
      'stream_infrared',
      'stream_fisheye',
      'stream_gyro',
      'stream_accel',
      'stream_gpio',
      'stream_pose',
      'stream_confidence',
    ];
    numberAttrs.forEach((attr) => {
      assert.equal(typeof obj[attr], 'number');
    });
    strAttrs.forEach((attr) => {
      assert.equal(typeof obj[attr], 'string');
    });
    for (let i = 0; i < obj.STREAM_COUNT; i++) {
      assert.equal(typeof obj.streamToString(i), 'string');
    }
  });
  it('camera_info test', () => {
    const obj = rs2.camera_info;
    const numberAttrs = [
      'CAMERA_INFO_NAME',
      'CAMERA_INFO_SERIAL_NUMBER',
      'CAMERA_INFO_FIRMWARE_VERSION',
      'CAMERA_INFO_RECOMMENDED_FIRMWARE_VERSION',
      'CAMERA_INFO_PHYSICAL_PORT',
      'CAMERA_INFO_DEBUG_OP_CODE',
      'CAMERA_INFO_ADVANCED_MODE',
      'CAMERA_INFO_PRODUCT_ID',
      'CAMERA_INFO_CAMERA_LOCKED',
      'CAMERA_INFO_USB_TYPE_DESCRIPTOR',
    ];
    const strAttrs = [
      'camera_info_name',
      'camera_info_serial_number',
      'camera_info_firmware_version',
      'camera_info_recommended_firmware_version',
      'camera_info_physical_port',
      'camera_info_debug_op_code',
      'camera_info_advanced_mode',
      'camera_info_product_id',
      'camera_info_camera_locked',
      'camera_info_usb_type_descriptor',
    ];
    numberAttrs.forEach((attr) => {
      assert.equal(typeof obj[attr], 'number');
    });
    strAttrs.forEach((attr) => {
      assert.equal(typeof obj[attr], 'string');
    });
    for (let i = 0; i < obj.CAMERA_INFO_COUNT; i++) {
      assert.equal(typeof obj.cameraInfoToString(i), 'string');
    }
    let ctx = new rs2.Context();
    let dev = ctx.queryDevices().devices[0];

    for (let i = obj.CAMERA_INFO_NAME; i < obj.CAMERA_INFO_COUNT; i++) {
      let info = dev.getCameraInfo(i);
      assert.equal((info === undefined) || (typeof info === 'string'), true);
    }
    rs2.cleanup();
  });
  it('option test', () => {
    const obj = rs2.option;
    const numberAttrs = [
      'OPTION_BACKLIGHT_COMPENSATION',
      'OPTION_BRIGHTNESS',
      'OPTION_CONTRAST',
      'OPTION_EXPOSURE',
      'OPTION_GAIN',
      'OPTION_GAMMA',
      'OPTION_HUE',
      'OPTION_SATURATION',
      'OPTION_SHARPNESS',
      'OPTION_WHITE_BALANCE',
      'OPTION_ENABLE_AUTO_EXPOSURE',
      'OPTION_ENABLE_AUTO_WHITE_BALANCE',
      'OPTION_VISUAL_PRESET',
      'OPTION_LASER_POWER',
      'OPTION_ACCURACY',
      'OPTION_MOTION_RANGE',
      'OPTION_FILTER_OPTION',
      'OPTION_CONFIDENCE_THRESHOLD',
      'OPTION_EMITTER_ENABLED',
      'OPTION_FRAMES_QUEUE_SIZE',
      'OPTION_TOTAL_FRAME_DROPS',
      'OPTION_AUTO_EXPOSURE_MODE',
      'OPTION_POWER_LINE_FREQUENCY',
      'OPTION_ASIC_TEMPERATURE',
      'OPTION_ERROR_POLLING_ENABLED',
      'OPTION_PROJECTOR_TEMPERATURE',
      'OPTION_OUTPUT_TRIGGER_ENABLED',
      'OPTION_MOTION_MODULE_TEMPERATURE',
      'OPTION_DEPTH_UNITS',
      'OPTION_ENABLE_MOTION_CORRECTION',
      'OPTION_AUTO_EXPOSURE_PRIORITY',
      'OPTION_COLOR_SCHEME',
      'OPTION_HISTOGRAM_EQUALIZATION_ENABLED',
      'OPTION_MIN_DISTANCE',
      'OPTION_MAX_DISTANCE',
      'OPTION_TEXTURE_SOURCE',
      'OPTION_FILTER_MAGNITUDE',
      'OPTION_FILTER_SMOOTH_ALPHA',
      'OPTION_FILTER_SMOOTH_DELTA',
      'OPTION_HOLES_FILL',
      'OPTION_STEREO_BASELINE',
      'OPTION_AUTO_EXPOSURE_CONVERGE_STEP',
      'OPTION_INTER_CAM_SYNC_MODE',
    ];
    const strAttrs = [
      'option_backlight_compensation',
      'option_brightness',
      'option_contrast',
      'option_exposure',
      'option_gain',
      'option_gamma',
      'option_hue',
      'option_saturation',
      'option_sharpness',
      'option_white_balance',
      'option_enable_auto_exposure',
      'option_enable_auto_white_balance',
      'option_visual_preset',
      'option_laser_power',
      'option_accuracy',
      'option_motion_range',
      'option_filter_option',
      'option_confidence_threshold',
      'option_emitter_enabled',
      'option_frames_queue_size',
      'option_total_frame_drops',
      'option_auto_exposure_mode',
      'option_power_line_frequency',
      'option_asic_temperature',
      'option_error_polling_enabled',
      'option_projector_temperature',
      'option_output_trigger_enabled',
      'option_motion_module_temperature',
      'option_depth_units',
      'option_enable_motion_correction',
      'option_auto_exposure_priority',
      'option_color_scheme',
      'option_histogram_equalization_enabled',
      'option_min_distance',
      'option_max_distance',
      'option_texture_source',
      'option_filter_magnitude',
      'option_filter_smooth_alpha',
      'option_filter_smooth_delta',
      'option_holes_fill',
      'option_stereo_baseline',
      'option_auto_exposure_converge_step',
      'option_inter_cam_sync_mode',
    ];
    numberAttrs.forEach((attr) => {
      assert.equal(typeof obj[attr], 'number');
    });
    strAttrs.forEach((attr) => {
      assert.equal(typeof obj[attr], 'string');
    });
    for (let i = 0; i < obj.OPTION_COUNT; i++) {
      assert.equal(typeof obj.optionToString(i), 'string');
    }
    let ctx = new rs2.Context();
    let sensors = ctx.querySensors();
    // make sure it would not crash
    sensors.forEach((s) => {
      for (let i = 0; i < obj.OPTION_COUNT; i++) {
        let old = s.getOption(i);
        s.setOption(i, 10);
        if (typeof old === 'number') {
          s.setOption(i, old);
        }
      }
    });
    rs2.cleanup();
  });
});

describe('infrared frame tests', function() {
  let ctx;
  let pipe;
  let cfg;
  before(function() {
    ctx = new rs2.Context();
    pipe = new rs2.Pipeline(ctx);
    cfg = new rs2.Config();
  });

  after(function() {
    rs2.cleanup();
  });

  it('getInfraredFrame', () => {
    cfg.enableStream(rs2.stream.STREAM_INFRARED, 1, 640, 480, rs2.format.FORMAT_Y8, 60);
    pipe.start(cfg);
    const frames = pipe.waitForFrames();
    assert.equal(frames.size === 1, true);
    let frame = frames.getInfraredFrame(1);
    assert.equal(frame.profile.streamType === rs2.stream.STREAM_INFRARED, true);
    assert.equal(frame.profile.streamIndex === 1, true);

    // Get the frame that first match
    frame = frames.getInfraredFrame();
    assert.equal(frame.profile.streamType === rs2.stream.STREAM_INFRARED, true);
    assert.equal(frame.profile.streamIndex === 1, true);
    pipe.stop();
  });
});

describe('Native error tests', function() {
  it('trigger native error test', () => {
    const file = 'test.rec';
    fs.writeFileSync(file, 'dummy');
    const ctx = new rs2.Context();
    assert.throws(() => {
      ctx.loadDevice(file);
    }, (err) => {
      assert.equal(err instanceof Error, true);
      let errInfo = rs2.getError();
      assert.equal(errInfo.recoverable, false);
      assert.equal(typeof errInfo.description, 'string');
      assert.equal(typeof errInfo.nativeFunction, 'string');
      return true;
    });
    rs2.cleanup();
    fs.unlinkSync(file);
  });
});

describe('ROI test', function() {
  it('set/get ROI test', () => {
    let ctx = new rs2.Context();
    let sensors = ctx.querySensors();
    sensors.forEach((s) => {
      let roi = rs2.ROISensor.from(s);
      if (roi) {
        roi.setRegionOfInterest(1, 1, 10, 10);
        let val = roi.getRegionOfInterest();
        if (val) {
          assert.equal(val.minX, 1);
          assert.equal(val.minY, 1);
          assert.equal(val.maxX, 10);
          assert.equal(val.maxY, 10);
        } else {
          assert.equal(rs2.getError() instanceof Object, true);
        }
      }
    });
    rs2.cleanup();
  });
});

describe('new record/playback test', function() {
  let pipe;
  let cfg;
  let device;
  const file = 'record.bag';

  it('record and playback', () => {
    // record
    pipe = new rs2.Pipeline();
    cfg = new rs2.Config();
    cfg.enableRecordToFile(file);
    pipe.start(cfg);
    device = pipe.getActiveProfile().getDevice();

    // make sure it's not a playback device
    let playback = rs2.PlaybackDevice.from(device);
    assert.equal(playback, undefined);

    let recorder = rs2.RecorderDevice.from(device);
    assert.equal(recorder instanceof rs2.RecorderDevice, true);
    pipe.waitForFrames();
    pipe.waitForFrames();
    pipe.waitForFrames();
    pipe.stop();
    // make sure the recorded frames are flushed to file
    rs2.cleanup();

    assert.equal(fs.existsSync(file), true);

    // playback
    cfg = new rs2.Config();
    cfg.enableDeviceFromFile(file);
    pipe = new rs2.Pipeline();
    pipe.start(cfg);
    device = pipe.getActiveProfile().getDevice();
    playback = rs2.PlaybackDevice.from(device);
    assert.equal(playback instanceof rs2.PlaybackDevice, true);

    // make sure it's not a RecorderDevice
    recorder = rs2.RecorderDevice.from(device);
    assert.equal(recorder, undefined);

    let frames = pipe.waitForFrames();
    assert.equal(frames instanceof rs2.FrameSet, true);
    pipe.stop();
    rs2.cleanup();
    fs.unlinkSync(file);
  }).timeout(10000);
});

describe('frameset misc test', function() {
  it('get any frame twice test', () => {
    let pipe = new rs2.Pipeline();
    pipe.start();
    let frames = pipe.waitForFrames();
    let frame = frames.getFrame(rs2.stream.STREAM_ANY);
    assert.equal(frame instanceof rs2.Frame, true);
    frame = frames.getFrame(rs2.stream.STREAM_ANY);
    assert.equal(frame instanceof rs2.Frame, true);
    pipe.stop();
    rs2.cleanup();
  });
});

describe('post processing filter tests', function() {
  let ctx;
  let pipe;
  before(function() {
    ctx = new rs2.Context();
    pipe = new rs2.Pipeline(ctx);
  });

  after(function() {
    rs2.cleanup();
  });

  it('hole-filling filter test', () => {
    pipe.start();
    const frames = pipe.waitForFrames();
    assert.equal(frames.size > 0, true);
    let filter = new rs2.HoleFillingFilter();
    let out = filter.process(frames.depthFrame);
    assert.equal(out instanceof rs2.Frame, true);
    assert.equal(out.isValid, true);
    pipe.stop();
  });
});
