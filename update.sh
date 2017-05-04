#!/bin/bash
set -euo pipefail

readonly tmpDir=/tmp/ld38-maaze-deliver-$$
trap "rm -rf $tmpDir" EXIT
mkdir -p $tmpDir

./check

#------------------------------------------------------------------------------
# create game directory
#------------------------------------------------------------------------------
readonly gameDir=$tmpDir/maaze
mkdir -p $gameDir

cp -a bin/asmjs/rel/* $gameDir
cp index.html $gameDir/index.html

#------------------------------------------------------------------------------
# archive it
#------------------------------------------------------------------------------
pushd $tmpDir
zip maaze.zip -r maaze
popd

mv $tmpDir/maaze.zip .

#------------------------------------------------------------------------------
# upload it to code.alaiwan.org
#------------------------------------------------------------------------------
rsync \
  --compress \
  --delete \
  -vr $gameDir/* alaiwans@code.alaiwan.org:public_html/games/maaze

