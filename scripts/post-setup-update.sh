#!/usr/bin/env sh
# Post-update setup for user-local filesystem on reMarkable Paper Pro 3.x

set -eu

BIN_DIR="$HOME/.local/bin"
LIB_DIR="$HOME/.local/lib"
PROFILE="$HOME/.profile"

say() { printf '%s\n' "$*"; }

ensure_dir() {
  [ -d "$1" ] || { mkdir -p "$1"; say "Created: $1"; }
}

ensure_line() {
  # $1=file, $2=exact line
  [ -f "$1" ] || touch "$1"
  if ! grep -Fqx -- "$2" "$1"; then
    printf '%s\n' "$2" >> "$1"
    say "Appended to $(basename "$1"): $2"
  fi
}

say "== post-update setup (mirtillo) =="

ensure_dir "$BIN_DIR"
ensure_dir "$LIB_DIR"

ensure_line "$PROFILE" 'export PATH="$HOME/.local/bin:$PATH"'
ensure_line "$PROFILE" 'export LD_LIBRARY_PATH="$HOME/.local/lib:$LD_LIBRARY_PATH"'

# reMarkable firmwares typically only provide "C" / "POSIX" locales.
ensure_line "$PROFILE" "export LANG=C"
ensure_line "$PROFILE" "export LC_ALL=C"
ensure_line "$PROFILE" "export QT_LOGGING_RULES='qt.core.locale.warning=false'"

say "Setup complete. Run: . $PROFILE"