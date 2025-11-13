#!/usr/bin/env bash
# Build and deploy mirtillo to reMarkable Paper Pro

set -euo pipefail

if ! command -v aarch64-remarkable-linux-g++ >/dev/null 2>&1; then
  echo "ERROR: SDK environment is not loaded."
  echo "Run first:"
  echo "  export QEMU_LD_PREFIX=\"\$HOME/rm-sdk/sysroots/cortexa53-crypto-remarkable-linux\""
  echo "  source \"\$HOME/rm-sdk/environment-setup-cortexa53-crypto-remarkable-linux\""
  exit 1
fi

PROJECT_ROOT="$(cd "$(dirname "$0")/.."; pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
TARGET_BIN="mirtillo"
RM_HOST="root@10.11.99.1"
RM_BIN_DIR="/home/root/.local/bin"

echo "== Building =="
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# CMake usa il cmake dell'SDK, già nel PATH grazie a:
#   export QEMU_LD_PREFIX=...
#   source "$HOME/rm-sdk/environment-setup-cortexa53-crypto-remarkable-linux"
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j

echo "== Deploying to ${RM_HOST}:${RM_BIN_DIR} =="
scp "$TARGET_BIN" "${RM_HOST}:${RM_BIN_DIR}/"

echo "Done."