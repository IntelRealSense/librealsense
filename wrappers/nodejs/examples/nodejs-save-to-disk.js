// Copyright (c) 2017 Intel Corporation. All rights reserved.
// Use of this source code is governed by an Apache 2.0 license
// that can be found in the LICENSE file.

'use strict';

const rs2 = require('../index.js');

let colorizer = new rs2.Colorizer();
let pipeline = new rs2.Pipeline();

pipeline.start();

for (let i = 0; i < 30; i++) {
  let frameset = pipeline.waitForFrames();
  frameset.destroy();
}

let frameset = pipeline.waitForFrames();
for (let i = 0; i < frameset.size; i++) {
  let frame = frameset.at(i);
  if (frame instanceof rs2.VideoFrame) {
    if (frame instanceof rs2.DepthFrame) {
      let newFrame = colorizer.colorize(frame);
      frame.destroy();
      frame = newFrame;
    }
    let pngFile = 'rs-save-to-disk-output-' +
                  rs2.stream.streamToString(frame.profile.streamType) + '.png';
    rs2.util.writeFrameToFile(pngFile, frame, 'png');
    console.log('Saved ', pngFile);
    let csvFile = 'rs-save-to-disk-output-' +
                  rs2.stream.streamToString(frame.profile.streamType) + '-metadata.csv';
    rs2.util.writeFrameMetadataToFile(csvFile, frame);
  }
  frame.destroy();
}
pipeline.stop();
pipeline.destroy();
frameset.destroy();
rs2.cleanup();
