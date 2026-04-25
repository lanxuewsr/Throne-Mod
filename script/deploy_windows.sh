#!/bin/bash
set -e

rm -rf $DEST
mkdir -p $DEST

#### copy exe ####
cp $GITHUB_WORKSPACE/build/Throne-Mod.exe $DEST
cp $GITHUB_WORKSPACE/build/Throne-Mod.pdb $DEST || true

ARTIFACT_TGZ=$(find "$GITHUB_WORKSPACE/download-artifact" -type f -name artifacts.tgz | head -n 1)
if [ -z "$ARTIFACT_TGZ" ]; then
  echo "artifacts.tgz not found under $GITHUB_WORKSPACE/download-artifact"
  exit 1
fi

ARTIFACT_DIR=$(dirname "$ARTIFACT_TGZ")
ARTIFACT_FILE=$(basename "$ARTIFACT_TGZ")
WORKSPACE_POSIX=$(cygpath -u "$GITHUB_WORKSPACE")

pushd "$ARTIFACT_DIR" >/dev/null
pushd "$WORKSPACE_POSIX" >/dev/null
tar xvzf "$(cygpath -u "$ARTIFACT_DIR")/$ARTIFACT_FILE"
popd >/dev/null
popd >/dev/null
