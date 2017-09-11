// Copyright (c) 2017 Intel Corporation. All rights reserved.
// Use of this source code is governed by an Apache 2.0 license
// that can be found in the LICENSE file.

'use strict';

const rs2 = require('../index.js');
const GLFWWindow = require('./glfw-window.js').GLFWWindow;
const glfw = require('./glfw-window.js').glfw;

// Open a GLFW window
const win = new GLFWWindow(1280, 720, 'Node.js Capture Example');
const colorizer = new rs2.Colorizer();
const pipeline = new rs2.Pipeline();
pipeline.start();

while (! win.shouldWindowClose()) {
  const frameset = pipeline.waitForFrames();

  const depth = frameset.depthFrame;
  const depthRGB = colorizer.colorize(depth);
  const color = frameset.colorFrame;

  win.beginPaint();
  glfw.draw2x2Streams(
      win.window,
      2, // two channels
      depthRGB ? depthRGB.getData() : null,
      'rgb8',
      depthRGB ? depthRGB.width : 0,
      depthRGB ? depthRGB.height : 0,
      color ? color.getData() : null,
      'rgb8',
      color ? color.width : 0,
      color ? color.height : 0,
      null, '', 0, 0,
      null, '', 0, 0);
  win.endPaint();

  if (depth) depth.destroy();
  if (depthRGB) depthRGB.destroy();
  if (color) color.destroy();

  frameset.destroy();
}

win.destroy();
rs2.cleanup();
