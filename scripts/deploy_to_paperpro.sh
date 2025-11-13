#!/usr/bin/env bash
# Build and deploy mirtillo to reMarkable Paper Pro

set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "$0")/.."; pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
TARGET_BIN="mirtillo"
RM_HOST="root@10.11.99.1"
RM_BIN_DIR="/home/root/.local/bin"
RM_SHARE_DIR="/home/root/.local/share/mirtillo"

echo "== Building =="
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j

echo "== Preparing remote dir =="
ssh "$RM_HOST" "mkdir -p '$RM_BIN_DIR' '$RM_SHARE_DIR'"

echo "== Deploying binary =="
scp "$TARGET_BIN" "${RM_HOST}:${RM_BIN_DIR}/"

echo "== Deploying ABOUT.txt =="
scp "${PROJECT_ROOT}/ABOUT.txt" "${RM_HOST}:${RM_SHARE_DIR}/ABOUT.txt"

echo "Done."