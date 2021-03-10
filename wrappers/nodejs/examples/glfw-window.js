// Copyright (c) 2017 Intel Corporation. All rights reserved.
// Use of this source code is governed by an Apache 2.0 license
// that can be found in the LICENSE file.

'use strict';

const glfw = require('node-glfw-3');
const rs2 = require('../index.js');
const now = require('performance-now');

class FPSCounter {
  // fps is reported to console for each interval frames
  constructor(interval = 10) {
    this.cnt = 0;
    this.interval = interval;
    this.timestamps = [];
  }

  count() {
    this.timestamps[this.cnt] = now();
    this.cnt++;
    if (this.cnt === this.interval) {
      this._report();
      this.reset();
    }
  }

  reset() {
    this.cnt = 0;
  }

  _report() {
    let duration = this.timestamps[this.cnt - 1] - this.timestamps[0];
    let fps = Math.round((this.cnt - 1) / (duration / 1000));
    console.log('FPS: ', fps);
  }
}

/* eslint-disable new-cap */
class GLFWWindow {
  // TODO(kenny-y): Check disabled jshint errors
  constructor(width = 1280, height = 720, title = 'GLFW Window') {
    this.width_ = width;
    this.height_ = height;

    glfw.Init();
    glfw.DefaultWindowHints();
    this.window_ = glfw.CreateGLFWWindow(width, height, title);
    glfw.MakeContextCurrent(this.window_);

    // Enable vertical sync (on cards that support it)
    glfw.SwapInterval(1); // 0 for vsync off
    this.enableFps = false;
    this.fpsCounter = new FPSCounter();
  }

  // Call this method to show or not to show fps to console
  showFPS(showOrNot) {
    this.enableFps = showOrNot;
    this.fpsCounter.reset();
  }

  get width() {
    return this.width_;
  }

  get height() {
    return this.height_;
  }

  get window() {
    return this.window_;
  }

  shouldWindowClose() {
    let result = glfw.WindowShouldClose(this.window_);
    if (! result) {
      result = glfw.GetKey(this.window_, glfw.KEY_ESCAPE);
    }
    return result;
  }

  beginPaint() {
    // We don't clear buffer for now, because
    //  there is currently sync issue between depth & color stream
    //
    // glfw.ClearColorBuffer();
  }

  endPaint() {
    glfw.PopMatrix();
    glfw.SwapBuffers(this.window);
    glfw.PollEvents();
    glfw.PushMatrix();
    glfw.Ortho(0, this.width_, this.height_, 0, -1, +1);
    if (this.enableFps) {
      this.fpsCounter.count();
    }
  }

  destroy() {
    glfw.DestroyWindow(this.window_);
    glfw.Terminate();
    this.window_ = undefined;
  }

  setKeyCallback(callback) {
    glfw.setKeyCallback(this.window_, function(key, scancode, action, modes) {
      callback(key, scancode, action, modes);
    });
  }
}

class Rect {
  constructor(x, y, w, h) {
    this.x = x;
    this.y = y;
    this.w = w;
    this.h = h;
  }

  adjustRatio(size) {
    let newH = this.h;
    let newW = this.h * size.x / size.y;
    if (newW > this.w) {
      let scale = this.w / newW;
      newW *= scale;
      newH *= scale;
    }
    return new Rect(this.x + (this.w-newW) / 2, this.y + (this.h - newH) / 2, newW, newH);
  }
}

class Texture {
  constructor() {
    this.glHandle = 0;
  }

  render(videoFrame, rect) {
    this.upload(videoFrame);
    this.show(rect);
  }

  upload(videoFrame) {
    if (!this.glHandle) {
      this.glHandle = glfw.genTexture();
    }
    glfw.uploadAsTexture(
        this.glHandle,
        new Uint8Array(videoFrame.getData().buffer),
        videoFrame.width,
        videoFrame.height,
        rs2.format.formatToString(videoFrame.format));
    this.stream = videoFrame.stream;
  }

  show(rect) {
    glfw.showInRect(this.glHandle, rect.x, rect.y, rect.w, rect.h);
  }
}

module.exports.GLFWWindow = GLFWWindow;
module.exports.glfw = glfw;
module.exports.Rect = Rect;
module.exports.Texture = Texture;
