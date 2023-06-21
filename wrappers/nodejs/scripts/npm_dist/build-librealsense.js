// Copyright (c) 2017 Intel Corporation. All rights reserved.
// Use of this source code is governed by an Apache 2.0 license
// that can be found in the LICENSE file.

// Purpose: build native librealsense

const {spawn} = require('child_process');
const os = require('os');

function buildNativeLib() {
  console.log('Building librealsense C++ API. It takes time...');
  const type = os.type();
  if (type === 'Windows_NT') {
    buildNativeLibWindows();
  } else if (type === 'Linux') {
    buildNativeLibLinux();
  } else if (type === 'Darwin') {
    buildNativeLibMac();
  } else {
    throw new TypeError('Not supported build platform!');
  }
}

function buildNativeLibMac() {
  let child = spawn('./scripts/npm_dist/build-dist-mac.sh');
  child.stderr.on('data', (data) => {
    console.error(`${data}`);
  });
  child.stdout.on('data', (data) => {
    console.log(`${data}`);
  });
  child.on('close', (code) => {
    if (code) {
      throw new Error(`Failed to build librealsense, build-dist-mac.sh exited with code ${code}`);
    }
  });
}

function buildNativeLibLinux() {
  let child = spawn('./scripts/npm_dist/build-dist.sh');
  child.stderr.on('data', (data) => {
    console.error(`${data}`);
  });
  child.stdout.on('data', (data) => {
    console.log(`${data}`);
  });
  child.on('close', (code) => {
    if (code) {
      throw new Error(`Failed to build librealsense, build-dist.sh exited with code ${code}`);
    }
  });
}

function buildNativeLibWindows() {
  const cmakeGenPlatform = process.arch;
  const msBuildPlatform = process.arch=='ia32'?'Win32':process.arch;
  let child = spawn('cmd', ['/c', '.\\scripts\\npm_dist\\build-dist.bat', cmakeGenPlatform,
      msBuildPlatform]);
  child.stderr.on('data', (data) => {
    console.error(`${data}`);
  });
  child.stdout.on('data', (data) => {
    console.log(`${data}`);
  });
  child.on('close', (code) => {
    if (code) {
      throw new Error(`Failed to build librealsense, build-dist.bat exited with code ${code}`);
    }
  });
}
buildNativeLib();
