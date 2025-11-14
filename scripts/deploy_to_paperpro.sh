#!/usr/bin/env bash
# mirtillo helper script
# Usage:
#   ./scripts/mirtillo.sh build [--setup]
#   ./scripts/mirtillo.sh deploy [--setup]
#   ./scripts/mirtillo.sh sshkeys [--setup]

set -euo pipefail

# -----------------------------
# Config
# -----------------------------
PROJECT_ROOT="$(cd "$(dirname "$0")/.."; pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
TARGET_BIN="mirtillo"

RM_HOST="${RM_HOST:-root@10.11.99.1}"
RM_BIN_DIR="/home/root/.local/bin"
RM_SHARE_DIR="/home/root/.local/share/mirtillo"

SSH_KEY="${SSH_KEY:-$HOME/.ssh/rm_key}"

# -----------------------------
# SSH wrappers
# -----------------------------
ssh_pw() { ssh "$RM_HOST" "$@"; }
scp_pw() { scp "$@"; }

ssh_key() { ssh -i "$SSH_KEY" "$RM_HOST" "$@"; }
scp_key() { scp -i "$SSH_KEY" "$@"; }

# -----------------------------
# Build
# -----------------------------
do_build() {
  echo "== Building mirtillo =="
  mkdir -p "$BUILD_DIR"
  cd "$BUILD_DIR"

  cmake .. -DCMAKE_BUILD_TYPE=Release
  cmake --build . -j

  echo "Build OK → $BUILD_DIR/$TARGET_BIN"
}

# -----------------------------
# Remote setup (only if requested)
# -----------------------------
do_setup() {
  local ssh_cmd="$1"

  echo "== Running setup on Paper Pro =="
  if ! $ssh_cmd "mkdir -p '$RM_BIN_DIR' '$RM_SHARE_DIR'"; then
    echo "ERROR: cannot create remote directories."
    echo "Did you run post-update-mirtillo-setup.sh on the device?"
    exit 1
  fi
  echo "Setup complete."
}

# -----------------------------
# Deploy
# -----------------------------
do_deploy() {
  local mode="$1"
  local setup="$2"

  local ssh_cmd scp_cmd
  if [ "$mode" = "pw" ]; then
    ssh_cmd=ssh_pw
    scp_cmd=scp_pw
  else
    ssh_cmd=ssh_key
    scp_cmd=scp_key
  fi

  # Optional setup
  if [ "$setup" = "yes" ]; then
    do_setup "$ssh_cmd"
  fi

  echo "== Deploying mirtillo =="
  $scp_cmd "$BUILD_DIR/$TARGET_BIN" "$PROJECT_ROOT/ABOUT.txt" \
      "$RM_HOST:$RM_SHARE_DIR/"

  $ssh_cmd "cp '$RM_SHARE_DIR/$TARGET_BIN' '$RM_BIN_DIR/'"

  echo "Deploy completed."
}

# -----------------------------
# Main
# -----------------------------
usage() {
  cat <<EOF
Usage:
  $0 build [--setup]
  $0 deploy [--setup]
  $0 sshkeys [--setup]

Description:
  build     Build and deploy using password
  deploy    Deploy only, using password
  sshkeys   Deploy only, via SSH key
  --setup   Create remote directories before deploying

EOF
}

cmd="${1:-}"
shift || true

setup="no"
for arg in "$@"; do
  if [ "$arg" = "--setup" ]; then
    setup="yes"
  else
    echo "Unknown option: $arg"
    usage
    exit 1
  fi
done

case "$cmd" in
  build)
    do_build
    do_deploy "pw" "$setup"
    ;;
  deploy)
    do_deploy "pw" "$setup"
    ;;
  sshkeys)
    do_deploy "key" "$setup"
    ;;
  *)
    usage
    exit 1
    ;;
esac