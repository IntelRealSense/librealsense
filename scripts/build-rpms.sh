#!/bin/bash

RPMBUILD_DIR="./rpmbuild"
RPMBUILD_SOURCES=$RPMBUILD_DIR/SOURCES

function die() {
	echo $1 >&2
	exit 1
}

root=$(git rev-parse --show-toplevel) || die "failed to get root of git tree"

cd $root
mkdir -p $RPMBUILD_SOURCES || die "failed create $RPMBUILD_SOURCES"

tag=$(git describe --abbrev=0 --tags) || die "failed to get latest tag"
ver=${tag:1}
ver_maj=${ver%.*}
ver_min=${ver##*.}
git archive --prefix librealsense-${ver}/ -o rpmbuild/SOURCES/librealsense-${ver}.tar.gz ${tag}
dnf builddep -y scripts/librealsense.spec
VERSION_MAJOR=$ver_maj VERSION_MINOR=$ver_min rpmbuild --define "_topdir ${PWD}/rpmbuild" -bb scripts/librealsense.spec
