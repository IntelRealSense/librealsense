#!/usr/bin/env node

// Copyright (c) 2017 Intel Corporation. All rights reserved.
// Use of this source code is governed by an Apache 2.0 license
// that can be found in the LICENSE file.

'use strict';

const rs2 = require('../index.js');
const {GLFWWindow} = require('./glfw-window.js');
const {glfw} = require('./glfw-window.js');

function drawPointCloud(win, color, points) {
  win.beginPaint();
  if (points.vertices && points.textureCoordinates ) {
    let count = points.size;
    glfw.drawDepthAndColorAsPointCloud(
        win.window,
        new Uint8Array(points.vertices.buffer),
        count,
        new Uint8Array(points.textureCoordinates.buffer),
        color.data,
        color.width,
        color.height,
        'rgb8');
  }
  win.endPaint();
}

// Open a GLFW window
const win = new GLFWWindow(1280, 720, 'Node.js PointCloud Example');
const pc = new rs2.PointCloud();
const pipeline = new rs2.Pipeline();

pipeline.start();

console.log('Drag to change perspective, scroll mouse wheel to zoom in/out.');

while (! win.shouldWindowClose()) {
  const frameSet = pipeline.waitForFrames();
  const pointsFrame = pc.calculate(frameSet.depthFrame);
  pc.mapTo(frameSet.colorFrame);
  drawPointCloud(win, frameSet.colorFrame, pointsFrame);
}

pc.destroy();
pipeline.stop();
pipeline.destroy();
win.destroy();

rs2.cleanup();
