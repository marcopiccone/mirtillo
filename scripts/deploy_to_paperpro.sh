#!/usr/bin/env bash
# Build and deploy mirtillo to reMarkable Paper Pro

set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "$0")/.."; pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
TARGET_BIN="mirtillo"
RM_HOST="root@10.11.99.1"
RM_BIN_DIR="/home/root/.local/bin"

echo "== Building =="
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE="${REMARKABLE_SDK:-$HOME/rm-toolchain}/toolchain.cmake"

cmake --build . -j

echo "== Deploying to ${RM_HOST}:${RM_BIN_DIR} =="
scp "$TARGET_BIN" "${RM_HOST}:${RM_BIN_DIR}/"

echo "Done."