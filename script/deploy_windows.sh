#!/bin/bash
set -e

rm -rf $DEST
mkdir -p $DEST

#### copy exe ####
cp $GITHUB_WORKSPACE/build/Throne-Mod.exe $DEST
cp $GITHUB_WORKSPACE/build/Throne-Mod.pdb $DEST || true

cd download-artifact
cd *$DEST_SUFFIX
tar xvzf artifacts.tgz -C ../../
cd ../..
