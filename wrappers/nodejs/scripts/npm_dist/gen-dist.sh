#!/bin/bash

# Copyright (c) 2017 Intel Corporation. All rights reserved.
# Use of this source code is governed by an Apache 2.0 license
# that can be found in the LICENSE file.

#Usage: gen-dist.sh xxxx.tar.gz

WORKDIR=`mktemp -d`
RAWMODULEDIR="node-librealsense/"
MODULEDIR="$WORKDIR/$RAWMODULEDIR"
RSDIR="$MODULEDIR/librealsense"

mkdir -p $RSDIR/wrappers
rsync -a ../.. $RSDIR --exclude wrappers --exclude doc --exclude unit-tests --exclude build --exclude .git
rsync -a . $MODULEDIR --exclude build --exclude dist --exclude node_modules

cp -f ../CMakeLists.txt $RSDIR/wrappers/
cp -f scripts/npm_dist/binding.gyp $MODULEDIR
cp -f scripts/npm_dist/package.json $MODULEDIR
cp -f scripts/npm_dist/README.md $MODULEDIR
git rev-parse --verify HEAD > $RSDIR/commit

pushd . > /dev/null
cd $WORKDIR
FILENAME=`npm pack $RAWMODULEDIR`
TARFILENAME="$WORKDIR/$FILENAME"

popd > /dev/null
mkdir -p dist
cp -f $TARFILENAME ./dist/
rm -rf $WORKDIR

echo "Generated ./dist/$FILENAME"
# SHOW INFO & DIR
if [ "$RS_SHOW_DIST_PACKAGE" == "1" ];
then
	gnome-open ./dist
fi
