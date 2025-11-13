#!/usr/bin/env sh
# Post-update setup for user-local filesystem + env vars (Paper Pro 3.23+)
# Safe to re-run; it won't duplicate lines.

set -eu

# --- Config (adjust if you like) ---
BIN_DIR="$HOME/.local/bin"
LIB_DIR="$HOME/.local/lib"
PROFILE="$HOME/.profile"      # su reMarkable è quello giusto
# -----------------------------------

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

say "== mirtillo post-update setup =="

# 1) Crea le cartelle locali standard
ensure_dir "$BIN_DIR"
ensure_dir "$LIB_DIR"

# 2) Variabili d'ambiente (PATH + librerie + UTF-8)
ensure_line "$PROFILE" 'export PATH="$HOME/.local/bin:$PATH"'
ensure_line "$PROFILE" 'export LD_LIBRARY_PATH="$HOME/.local/lib:$LD_LIBRARY_PATH"'
ensure_line "$PROFILE" 'export LANG=C.UTF-8'
ensure_line "$PROFILE" 'export LC_ALL=C.UTF-8'
ensure_line "$PROFILE" 'export LC_CTYPE=C.UTF-8'

# 3) Messaggi finali
say ""
say "Setup complete."
say "• Binari:  $BIN_DIR"
say "• Librerie: $LIB_DIR"
say "• Env vars added to: $PROFILE"
say ""
say "Apri una nuova sessione SSH (o esegui:  . $PROFILE ) per attivare l'ambiente."