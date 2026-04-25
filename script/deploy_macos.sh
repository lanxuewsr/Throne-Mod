#!/bin/bash
set -e

rm -rf $DEST
mkdir -p $DEST

#### copy golang => .app ####
cd download-artifact
cd *$DEST_SUFFIX
tar xvzf artifacts.tgz -C ../../
cd ../..

mv deployment/$DEST_SUFFIX/* $GITHUB_WORKSPACE/build/Throne-Mod.app/Contents/MacOS

#### deploy qt & Dylib runtime => .app ####
pushd $GITHUB_WORKSPACE/build
macdeployqt Throne-Mod.app -verbose=3
popd

codesign --force --deep --sign - $GITHUB_WORKSPACE/build/Throne-Mod.app

dsymutil $GITHUB_WORKSPACE/build/Throne-Mod.app/Contents/MacOS/Throne-Mod
strip -S $GITHUB_WORKSPACE/build/Throne-Mod.app/Contents/MacOS/Throne-Mod

mv $GITHUB_WORKSPACE/build/Throne-Mod.app $DEST
