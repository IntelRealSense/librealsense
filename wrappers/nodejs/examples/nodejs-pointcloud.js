// Copyright (c) 2017 Intel Corporation. All rights reserved.
// Use of this source code is governed by an Apache 2.0 license
// that can be found in the LICENSE file.

'use strict';

const rs2 = require('../index.js');
const GLFWWindow = require('./glfw-window.js').GLFWWindow;
const glfw = require('./glfw-window.js').glfw;

function drawPointcloud(win, color, points) {
  let vertices = points.getVertices();
  let texCoords = points.getTextureCoordinates();
  let count = points.size;
  let colorWidth = color?color.width:0;
  let colorHeight = color?color.height:0;
  win.beginPaint();
  glfw.drawDepthAndColorAsPointCloud(
      win.window,
      vertices,
      count,
      texCoords,
      color ? color.getData() : null,
      colorWidth,
      colorHeight,
      'rgb8');
  win.endPaint();
}

// Open a GLFW window
const win = new GLFWWindow(1280, 720, 'Node.js Pointcloud Example');
const context = new rs2.Context();
const pc = context.createPointcloud();
const pipe = new rs2.Pipeline(context);
pipe.start();
while (! win.shouldWindowClose()) {
  const frameset = pipe.waitForFrames();
  if (!frameset) continue;

  let points;
  let color = frameset.colorFrame;
  let depth = frameset.depthFrame;

  if (color) pc.mapTo(color);
  if (depth) points = pc.calculate(depth);
  if (points) drawPointcloud(win, color, points);
  if (depth) depth.destroy();
  if (color) color.destroy();

  frameset.destroy();
}
win.destroy();
rs2.cleanup();
