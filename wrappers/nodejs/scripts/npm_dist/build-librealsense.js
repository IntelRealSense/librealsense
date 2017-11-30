// Copyright (c) 2017 Intel Corporation. All rights reserved.
// Use of this source code is governed by an Apache 2.0 license
// that can be found in the LICENSE file.

// Purpose: build native librealsense

const {exec} = require('child_process');
const os = require('os');

function buildNativeLib() {
  console.log('Building librealsense C++ API. It takes time...');

  if (os.type() == 'Windows_NT') {
    buildNativeLibWindows();
  } else {
    buildNativeLibLinux();
  }
}

function buildNativeLibLinux() {
  exec('./scripts/npm_dist/build-dist.sh',
       (error, stdout, stderr) => {
    if (error) {
      console.log('Failed to build librealsense, error:', error);
      throw error;
    }

    if (stdout) {
      process.stdout.write(stdout);
    }

    if (stderr) {
      process.stderr.write(stderr);
    }
  });
}

function buildNativeLibWindows() {
  const cmakeGenPlatform = process.arch;
  const msBuildPlatform = process.arch=='ia32'?'Win32':process.arch;

  exec('cmd /c .\\scripts\\npm_dist\\build-dist.bat ' + cmakeGenPlatform + ' ' + msBuildPlatform,
       (error, stdout, stderr) => {
    if (stdout) {
      process.stdout.write(stdout);
    }

    if (stderr) {
      process.stderr.write(stderr);
    }

    if (error) {
      console.log('Failed to build librealsense, error:', error);
      throw error;
    }
  });
}

buildNativeLib();
