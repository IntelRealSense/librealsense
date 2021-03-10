#!/usr/bin/env node

// Copyright (c) 2017 Intel Corporation. All rights reserved.
// Use of this source code is governed by an Apache 2.0 license
// that can be found in the LICENSE file.

'use strict';

const rs2 = require('../index.js');
const {GLFWWindow} = require('./glfw-window.js');
const {Rect} = require('./glfw-window.js');
const {Texture} = require('./glfw-window.js');

function tryGetDepthScale(dev) {
  const sensors = dev.querySensors();
  for (let i = 0; i < sensors.length; i++) {
    if (sensors[i] instanceof rs2.DepthSensor) {
      return sensors[i].depthScale;
    }
  }
  return undefined;
}

function removeBackground(otherFrame, depthFrame, depthScale, clippingDist) {
  let depthData = depthFrame.getData();
  let otherData = otherFrame.getData();
  const width = otherFrame.width;
  const height = otherFrame.height;
  const otherBpp = otherFrame.bytesPerPixel;

  for (let y = 0; y < height; y++) {
    let depthPixelIndex = y * width;
    for (let x = 0; x < width; x++, ++depthPixelIndex) {
      let pixelDistance = depthScale * depthData[depthPixelIndex];
      if (pixelDistance <= 0 || pixelDistance > clippingDist) {
        let offset = depthPixelIndex * otherBpp;

        // Set pixel to background color
        for (let i = 0; i < otherBpp; i++) {
          otherData[offset+i] = 0x99;
        }
      }
    }
  }
}

// Open a GLFW window
const win = new GLFWWindow(1280, 720, 'Node.js Align Example');

const colorizer = new rs2.Colorizer();
const renderer = new Texture();
const align = new rs2.Align(rs2.stream.STREAM_COLOR);
const pipeline = new rs2.Pipeline();
const profile = pipeline.start();

const depthScale = tryGetDepthScale(profile.getDevice());
if (depthScale === undefined) {
  console.error('Device does not have a depth sensor');
  process.exit(1);
}

let clippingDistance = 1.0;
console.log('Press Up/Down to change the depth clipping distance.');

win.setKeyCallback((key, scancode, action, modes) => {
  if (action != 0) return;

  if (key === 265) {
    // Pressed: Up arrow key
    clippingDistance += 0.1;
    if (clippingDistance > 6.0) {
      clippingDistance = 6.0;
    }
  } else if (key === 264) {
    // Pressed: Down arrow key
    clippingDistance -= 0.1;
    if (clippingDistance < 0) {
      clippingDistance = 0;
    }
  }
  console.log('Depth clipping distance:', clippingDistance.toFixed(3));
});

while (!win.shouldWindowClose()) {
  const rawFrameset = pipeline.waitForFrames();
  const alignedFrameset = align.process(rawFrameset);
  let colorFrame = alignedFrameset.colorFrame;
  let depthFrame = alignedFrameset.depthFrame;

  if (!depthFrame) {
    continue;
  }

  removeBackground(colorFrame, depthFrame, depthScale, clippingDistance);
  let w = win.width;
  let h = win.height;

  let colorRect = new Rect(0, 0, w, h);
  colorRect = colorRect.adjustRatio({
      x: colorFrame.width,
      y: colorFrame.height});

  let pipStream = new Rect(0, 0, w / 5, h / 5);
  pipStream = pipStream.adjustRatio({
      x: depthFrame.width,
      y: depthFrame.height});
  pipStream.x = colorRect.x + colorRect.w - pipStream.w - (Math.max(w, h) / 25);
  pipStream.y = colorRect.y + colorRect.h - pipStream.h - (Math.max(w, h) / 25);

  const colorizedDepth = colorizer.colorize(depthFrame);

  win.beginPaint();
  renderer.render(colorFrame, colorRect);
  renderer.upload(colorizedDepth);
  renderer.show(pipStream);
  win.endPaint();
}

pipeline.stop();
pipeline.destroy();
align.destroy();

win.destroy();
rs2.cleanup();
