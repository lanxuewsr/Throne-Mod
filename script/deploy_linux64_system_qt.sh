#!/bin/bash
set -e

rm -rf $DEST
mkdir -p $DEST

#### copy binary ####
cp $GITHUB_WORKSPACE/build/Throne-Mod $DEST

#### copy Throne.png ####
cp $GITHUB_WORKSPACE/res/public/Throne.png $DEST

#### copy Core ####
cd download-artifact
cd *${DEST_SUFFIX%-system-qt}
tar xvzf artifacts.tgz -C ../../
cd ../..
cp deployment/${DEST_SUFFIX%-system-qt}/Throne-ModCore $DEST
rm -rf deployment/${DEST_SUFFIX%-system-qt}

# handle debug info
objcopy --only-keep-debug $DEST/Throne-Mod $DEST/Throne-Mod.debug
strip --strip-debug --strip-unneeded $DEST/Throne-Mod
objcopy --add-gnu-debuglink=$DEST/Throne-Mod.debug $DEST/Throne-Mod
