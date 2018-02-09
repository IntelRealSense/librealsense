#!/usr/bin/env node

// Copyright (c) 2018 Intel Corporation. All rights reserved.
// Use of this source code is governed by an Apache 2.0 license
// that can be found in the LICENSE file.

const FSM = require('fsm-as-promised');
const input = require('input');
const rs2 = require('../../index.js');
const {GLFWWindow, Texture, Rect} = require('../glfw-window.js');

let devices;
let dev;
let sensor;
let action;

function printDeviceInfo(device) {
  const info = device.getCameraInfo();
  if (info.name.indexOf('RealSense') != -1) {
    console.log('Device Infomation: \n', info);
  }
}

function selectDevImpl(descriptions) {
  if (devices.length === 1) {
    dev = devices[0];
    console.log(getDeviceName(dev) + ' selected');
    return Promise.resolve(dev);
  }
  const deviceArray = [];
  devices.forEach((dev, index) => {
    deviceArray.push(index.toString() + ' ' + getDeviceName(dev));
  });
  return getUserSelection(descriptions, deviceArray).then((idx) => {
    dev = devices[idx];
    return dev;
  });
}

function selectSensorImpl(descriptions) {
  const sensors = dev.querySensors();
  const moreDescription = 'Device consists of ' + sensors.length + ' sensors.';
  if (sensors.length === 1) {
    sensor = sensors[0];
    console.log(moreDescription + getSensorName(sensor) + ' selected');
    return Promise.resolve(sensor);
  }
  const sensorArray = [];
  sensors.forEach((sensor, index) => {
    sensorArray.push(index.toString() + ' ' + getSensorName(sensor));
  });
  descriptions.push(moreDescription);
  return getUserSelection(descriptions, sensorArray).then((idx) => {
    sensor = sensors[idx];
    return sensor;
  });
}

const actions = [
  {description: 'Control sensor\'s options', operation: controlSensorOptionsAction},
  {description: 'Control sensor\'s streams', operation: controlSensorStreamsAction},
  {description: 'Show stream intrinsics', operation: showStreamIntrinsicsAction},
  {description: 'Show extrinsics between streams', operation: displayExtrinsicsAction},
];

function changeSensorOption(option) {
  console.log('Supported range for opton ', rs2.option.optionToString(option), ':');
  console.log(sensor.getOptionRange(option));
  return input.confirm('Change option\'s value?').then((ret) => {
    if (ret) {
      return input.text('Enter the new value for this option:');
    }
  }).then((val) => {
    if (val) {
      sensor.setOption(option, Number.parseFloat(val));
    }
  });
}

function controlSensorOptionsAction() {
  const choices = [];
  const options = [];
  for (let i = rs2.option.OPTION_BACKLIGHT_COMPENSATION; i < rs2.option.OPTION_COUNT; i++) {
    if (sensor.supportsOption(i)) {
      choices.push(i.toString() + ': ' + 'Description: ' + sensor.getOptionDescription(i) +
          ' Current Value: ' + sensor.getOption(i));
      options.push(i);
    }
  }
  return getUserSelection(
      ['Choose an option between the supported options'], choices).then((idx) => {
    return changeSensorOption(options[idx]);
  });
}

function selectStreamProfile(descriptions, sens, filter) {
  const profiles = sens.getStreamProfiles();
  const choices = [];
  const indexes = [];
  profiles.forEach((profile, index) => {
    if (filter && !filter(profile)) {
      return;
    }
    let streamInfo = index.toString() + ' ' +
        rs2.stream.streamToString(profile.streamType) + '#' + profile.streamIndex;
    if (profile instanceof rs2.VideoStreamProfile) {
      streamInfo += '(Video Stream: ' + rs2.format.formatToString(profile.format) + ' ' +
      profile.width + 'x' + profile.height + '@' + profile.fps + 'Hz';
    }
    choices.push(streamInfo);
    indexes.push(index);
  });
  return getUserSelection(descriptions, choices).then((idx) => {
    return profiles[indexes[idx]];
  });
}

function startStreamProfle(profile) {
  return new Promise((resolve, reject) => {
    const win = new GLFWWindow(
        profile.width ? profile.width : 1280,
        profile.height ? profile.height : 720,
        'Displaying profile ' + rs2.stream.streamToString(profile.streamType));
    const colorizer = new rs2.Colorizer();
    const decimate = new rs2.DecimationFilter();
    const paintRect = new Rect(0, 0, profile.width, profile.height);
    sensor.open(profile);
    sensor.start((frame) => {
      if (!win.shouldWindowClose()) {
        let resultFrame = frame;
        if (frame instanceof rs2.DepthFrame) {
          resultFrame = colorizer.colorize(decimate.process(frame));
        }
        win.beginPaint();
        renderer.render(resultFrame, paintRect);
        win.endPaint();
      } else {
        setTimeout(() => {
          sensor.stop();
          win.destroy();
          resolve();
        }, 0);
      }
    });
  });
}

function controlSensorStreamsAction() {
  return selectStreamProfile(['Please select the desired stream profile'], sensor, (profile) => {
    if (profile.streamType === rs2.stream.STREAM_DEPTH ||
        (profile.streamType === rs2.stream.STREAM_COLOR &&
        profile.format === rs2.format.FORMAT_RGB8)) {
      return true;
    }
    return false;
  }).then((profile) => {
    return startStreamProfle(profile);
  });
}

function showStreamIntrinsicsAction() {
  return selectStreamProfile(
      ['Please select the desired stream profile'], sensor).then((profile) => {
    if (profile instanceof rs2.VideoStreamProfile) {
      console.log('Intrinsics is:\n\n', profile.getIntrinsics());
    } else if (profile instanceof rs2.MotionStreamProfile) {
      console.log('Motion intrinsics is:\n\n', profile.getMotionIntrinsics());
    }
  });
}

function displayExtrinsicsAction(device, sensor) {
  let fromSensor;
  let fromProfile;
  let toSensor;
  let toProfile;
  let fromDescriptions = [
    'Select a sensor as the from sensor',
    // eslint-disable-next-line
    'Please choose a sensor and then a stream that will be used as the origin of extrinsic transformation'
  ];
  let toDescriptions = [
    'Select a sensor as the target sensor',
    // eslint-disable-next-line
    'Please choose a sensor and then a stream that will be used as the target of extrinsic transformation'
  ];
  return selectSensorImpl(fromDescriptions).then((s1) => {
    fromSensor = s1;
    return selectStreamProfile(['Select a profile as the from profile'], fromSensor);
  }).then((p1) => {
    fromProfile = p1;
    return selectSensorImpl(toDescriptions);
  }).then((s2) => {
    toSensor = s2;
    return selectStreamProfile(['Select a profile as the target profile'], toSensor);
  }).then((p2) => {
    toProfile = p2;
    console.log('Extrinsics is:\n\n', fromProfile.getExtrinsicsTo(toProfile));
  });
}

function selectActionImpl(descriptions) {
  const actionChoices = [];
  actions.forEach((act, index) => {
    actionChoices.push(index.toString() + ' ' + act.description);
  });
  return getUserSelection(descriptions, actionChoices).then((index) => {
    action = actions[index];
  });
}

function connectDevice() {
  const ctx = new rs2.Context();
  devices = ctx.queryDevices().devices;
  if (!devices) {
    console.log('No devices connected, waiting for device to be connected...');
    const hub = new rs2.DeviceHub(ctx);
    return [hub.waitForDevice()];
  }
  return devices;
}

function getDeviceName(device) {
  let name = 'Unknown device';
  if (device.supportsCameraInfo(rs2.camera_info.CAMERA_INFO_NAME)) {
    name = device.getCameraInfo(rs2.camera_info.CAMERA_INFO_NAME);
  }
  if (device.supportsCameraInfo(rs2.camera_info.CAMERA_INFO_SERIAL_NUMBER)) {
    sn = device.getCameraInfo(rs2.camera_info.CAMERA_INFO_SERIAL_NUMBER);
  }
  return name + ' #' + sn;
}

function getSensorName(sensor) {
  if (sensor.supportsCameraInfo(rs2.camera_info.CAMERA_INFO_NAME)) {
    return sensor.getCameraInfo(rs2.camera_info.CAMERA_INFO_NAME);
  } else {
    return 'Unknown Sensor';
  }
}

function getUserSelection(descriptionArray, choices) {
  console.log('\n\n================ ' + descriptionArray[0] + ' ============================');
  for (let i = 1; i < descriptionArray.length; i++) {
    console.log(descriptionArray[i]);
  }
  return input.select('Please select', choices).then((result) => {
    for (let i = 0; i < choices.length; i++) {
      if (choices[i] === result) {
        return i;
      }
    }
  });
}
const renderer = new Texture();
machine = new FSM({
  initial: 'stateInit',
  events: [
    {name: 'transSelectDev', from: 'stateInit', to: 'stateDevSelected'},
    {name: 'transSelectSensor', from: 'stateDevSelected', to: 'stateSensorSelected'},
    {name: 'transSelectAction', from: 'stateSensorSelected', to: 'stateActionSelected'},
    {name: 'transEnd', from: 'stateActionSelected', to: 'stateEnd'},
    {name: 'transReturnToInit', from: 'stateDevSelected', to: 'stateInit'},
    {name: 'transReturnToDevSelected', from: 'stateSensorSelected', to: 'stateDevSelected'},
    {name: 'transReturnToSensorSelected', from: 'stateActionSelected', to: 'stateSensorSelected'},
  ],
  callbacks: {
    ontransSelectDev: () => {
      return selectDevImpl(['Select a device']);
    },
    ontransSelectSensor: () => {
      return selectSensorImpl(['Select a sensor']);
    },
    ontransSelectAction: () => {
      return selectActionImpl(['What would you like to do with the sensor ?']);
    },
    onenteredstateInit: () => {
      return machine.transSelectDev();
    },
    onenteredstateDevSelected: () => {
      printDeviceInfo(dev);
      if (devices.length === 1) {
        return machine.transSelectSensor();
      }
      const choices = ['select a sensor', 'return to select other device'];
      const choiceActions = ['transSelectSensor', 'transReturnToInit'];
      return getUserSelection(['Choose what to do next'], choices).then((idx) => {
        return machine[choiceActions[idx]]();
      });
    },
    onenteredstateSensorSelected: () => {
      const choices = ['select an action', 'return to select other sensor'];
      const choiceActions = ['transSelectAction', 'transReturnToDevSelected'];
      return getUserSelection(['Choose what to do next'], choices).then((idx) => {
        return machine[choiceActions[idx]]();
      });
    },
    onenteredstateActionSelected: () => {
      const choices = ['quit', 'return to select other action'];
      const choiceActions = ['transEnd', 'transReturnToSensorSelected'];
      return action.operation().then(() => {
        return getUserSelection(['Choose what to do next'], choices);
      }).then((idx) => {
        return machine[choiceActions[idx]]();
      });
    },
    onenteredstateEnd: () => {
      rs2.cleanup();
    },
  },
});

connectDevice();
machine.transSelectDev();
