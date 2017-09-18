#!/bin/bash

# Copyright (c) 2017 Intel Corporation. All rights reserved.
# Use of this source code is governed by an Apache 2.0 license
# that can be found in the LICENSE file.

#Usage: gen-dist.sh xxxx.tar.gz

WORKDIR=`mktemp -d`
RAWMODULEDIR="node-librealsense"
MODULEDIR="$WORKDIR/$RAWMODULEDIR"
RSDIR="$MODULEDIR/librealsense"
TARFILENAME="$WORKDIR/$1"

mkdir -p $RSDIR
rsync -a ../.. $RSDIR --exclude wrappers --exclude doc --exclude unit-tests --exclude build --exclude .git
rsync -a . $MODULEDIR --exclude build --exclude dist --exclude node_modules

cp -f scripts/npm_dist/binding.gyp $MODULEDIR
cp -f scripts/npm_dist/package.json $MODULEDIR

pushd . > /dev/null
cd $WORKDIR
tar -czf $1 $RAWMODULEDIR

popd > /dev/null
mkdir -p dist
cp -f $TARFILENAME ./dist/
rm -rf $WORKDIR

# SHOW INFO & DIR
if [ "$RS_SHOW_DIST_PACKAGE" -eq 1 ]
then
	echo "Generated ./dist/$1"
	gnome-open ./dist
fi
