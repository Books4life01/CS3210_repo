#!/bin/sh
set -eu
SESSION="xv6_gdb"
CONTAINER="xv6"
DOCKER_SCRIPT="./scripts/docker.sh"
BUILD_DIR="./build"

err() {
  echo "error: $*" >&2
  exit 1
}

# ---- sanity checks ----
command -v tmux >/dev/null 2>&1 || err "tmux not found"
command -v docker >/dev/null 2>&1 || err "docker not found"
docker info >/dev/null 2>&1 || err "docker daemon not running"

[ -f "$DOCKER_SCRIPT" ] || err "not in project root"
[ -x "$DOCKER_SCRIPT" ] || err "docker.sh not executable"
[ -d "$BUILD_DIR" ] || err "missing build directory"

# ---- tmux session ----
if tmux has-session -t "$SESSION" 2>/dev/null; then
  echo "tmux session already exists. reusing."
else
  tmux new-session -d -s "$SESSION"
  tmux set-option -t "$SESSION" -g mouse on
fi

CURRENT_WINDOW=""
if [ -n "${TMUX:-}" ]; then
  CURRENT_SESSION=$(tmux display-message -p '#{session_name}')
  CURRENT_WINDOW=$(tmux display-message -p '#{window_index}')
  
  if [ "$CURRENT_SESSION" = "$SESSION" ]; then
    tmux kill-pane -a -t "$SESSION"
    WINDOW_ID="$SESSION:$CURRENT_WINDOW"
  else
    # create a fresh window at ID 0 named 'gdb'
    WINDOW_ID=$(tmux new-window -t "$SESSION" -n gdb -P -F '#{window_id}')
  fi
else
    # create a fresh window at ID 0 named 'gdb'
    WINDOW_ID=$(tmux new-window -t "$SESSION" -n gdb -P -F '#{window_id}')
fi

# get authoritative left pane
LEFT_PANE=$(tmux list-panes -t "$WINDOW_ID" -F '#{pane_id}')
RIGHT_PANE=$(tmux split-window -h -t "$LEFT_PANE" -P -F '#{pane_id}')

# container state
sleep 1   # give time for docker container to reset
if docker ps --format '{{.Names}}' | grep -qx "$CONTAINER"; then
  CONTAINER_RUNNING=true
else
  CONTAINER_RUNNING=false
fi

# ---- left pane ----
if [ "$CONTAINER_RUNNING" = false ]; then
  tmux send-keys -t "$LEFT_PANE" "$DOCKER_SCRIPT" C-m
else
  echo "note: container already running, left pane will reuse it" >&2
  tmux send-keys -t "$LEFT_PANE" "$DOCKER_SCRIPT --attach" C-m
fi
tmux send-keys -t "$LEFT_PANE" "cd build" C-m
tmux send-keys -t "$LEFT_PANE" "make" C-m
tmux send-keys -t "$LEFT_PANE" "./xv6-qemu -g" C-m

# ---- right pane ----
tmux send-keys -t "$RIGHT_PANE" "while ! docker ps --format '{{.Names}}' | grep -qx $CONTAINER; do sleep 0.1; done" C-m
tmux send-keys -t "$RIGHT_PANE" "$DOCKER_SCRIPT --attach" C-m
tmux send-keys -t "$RIGHT_PANE" "cd build" C-m
tmux send-keys -t "$RIGHT_PANE" "gdb" C-m

tmux attach -t "$SESSION"
