// Copyright (c) 2017 Intel Corporation. All rights reserved.
// Use of this source code is governed by an Apache 2.0 license
// that can be found in the LICENSE file.

'use strict';

const {exec} = require('child_process');
const {chdir} = require('process');
const fsCompare = require('fs-compare');

chdir('doc');

fsCompare.ctime('../index.js', './index.html', function(err, diff) {
  if (err) {
    console.log(err);
    throw err;
  } else {
    if (diff == 1) {
      console.log('Doc: rebuild');
      genDoc();
    } else {
      console.log('Doc: already up to date');
    }
  }
});

function genDoc() {
  exec('jsdoc ../index.js -t ./jsdoc-template -d .', (error, stdout, stderr) => {
    if (error) {
      throw error;
    }

    if (stdout) process.stdout.write(stdout);
    if (stderr) process.stderr.write(stderr);
  });
}
