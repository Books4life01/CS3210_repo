#!/bin/sh
set -eu

SESSION="xv6_gdb"
CONTAINER="xv6"
BUILD_DIR="build"
DOCKER_SCRIPT="./scripts/docker.sh"

err() {
  echo "error: $*" >&2
  exit 1
}

# ---- sanity checks ----

command -v tmux >/dev/null 2>&1 || err "tmux not found. please install it."
command -v docker >/dev/null 2>&1 || err "docker not found. please install it."

# docker daemon running?
docker info >/dev/null 2>&1 || err "docker daemon not running"

# at root dir?
[ -f "$DOCKER_SCRIPT" ] || err "not in project root (missing $DOCKER_SCRIPT)"
[ -x "$DOCKER_SCRIPT" ] || err "$DOCKER_SCRIPT exists but is not executable"

# project layout
[ -d "$BUILD_DIR" ] || err "missing ./$BUILD_DIR directory"

# container state
if docker ps --format '{{.Names}}' | grep -qx "$CONTAINER"; then
  CONTAINER_RUNNING=true
else
  CONTAINER_RUNNING=false
fi

# ---- tmux session ----

if tmux has-session -t "$SESSION" 2>/dev/null; then
  err "tmux session '$SESSION' already exists"
fi

tmux new-session -d -s "$SESSION"
tmux set-option -t "$SESSION" -g mouse on

LEFT_PANE=$(tmux list-panes -t "$SESSION" -F '#{pane_id}')
RIGHT_PANE=$(tmux split-window -h -t "$LEFT_PANE" -P -F '#{pane_id}')

# ---- left pane ----
if [ "$CONTAINER_RUNNING" = false ]; then
  tmux send-keys -t "$LEFT_PANE" "$DOCKER_SCRIPT" C-m
else
  echo "note: container already running, left pane will reuse it" >&2
  tmux send-keys -t "$LEFT_PANE" "$DOCKER_SCRIPT --attach" C-m
fi

tmux send-keys -t "$LEFT_PANE" "cd $BUILD_DIR" C-m
tmux send-keys -t "$LEFT_PANE" "make" C-m
tmux send-keys -t "$LEFT_PANE" "./xv6-qemu -g" C-m

sleep 1

# ---- right pane ----
tmux send-keys -t "$RIGHT_PANE" "$DOCKER_SCRIPT --attach" C-m
tmux send-keys -t "$RIGHT_PANE" "cd $BUILD_DIR" C-m
tmux send-keys -t "$RIGHT_PANE" "gdb" C-m

tmux attach -t "$SESSION"

