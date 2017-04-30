#!/bin/bash
set -euo pipefail

readonly tmpDir=/tmp/ld38-naarrow-deliver-$$
trap "rm -rf $tmpDir" EXIT
mkdir -p $tmpDir

./check

#------------------------------------------------------------------------------
# create game directory
#------------------------------------------------------------------------------
readonly gameDir=$tmpDir/naarrow
mkdir -p $gameDir

rsync \
  --compress \
  --delete-excluded \
  --exclude "*.dep" \
  --exclude "*.bc" \
  --exclude "*.o" \
  --exclude "*.deps" \
  -vr bin/asmjs/* $gameDir
cp index.html $gameDir/index.html

#------------------------------------------------------------------------------
# archive it
#------------------------------------------------------------------------------
pushd $tmpDir
zip naarrow.zip -r naarrow
popd

mv $tmpDir/naarrow.zip .

#------------------------------------------------------------------------------
# upload it to code.alaiwan.org
#------------------------------------------------------------------------------
rsync \
  --compress \
  --delete \
  -vr $gameDir/* alaiwans@code.alaiwan.org:public_html/games/naarrow

