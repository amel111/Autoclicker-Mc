#!/bin/bash

# Log everything to help debugging
exec 1>>/tmp/autoclicker_script.log 2>&1
echo "=== $(date) - Script Triggered: $1 $2 ==="

export YDOTOOL_SOCKET=/tmp/.ydotool_socket
MODE=${1:-left}
ACTION=${2:-toggle}

if [ "$MODE" = "left" ]; then
    RUNFILE="/tmp/autoclicker_left.run"
    BUTTON="0xC0"
    LABEL="Left"
else
    RUNFILE="/tmp/autoclicker_right.run"
    BUTTON="0xC1"
    LABEL="Right"
fi

# Stop action: always write 0, regardless of focus. This fixes race conditions
# where multiple scripts start/stop at exactly the same time.
if [ "$ACTION" = "stop" ]; then
    echo "0" > "$RUNFILE"
    echo "Wrote 0 to $RUNFILE to stop autoclicker."
    notify-send "Autoclicker" "$LABEL OFF 🔴" --expire-time=1000
    exit 0
fi

# Start action: only start if Minecraft is focused
FOCUSED=$(hyprctl activewindow | grep "class:" | awk '{print $2}')
echo "Focused window is: '$FOCUSED'"

if [ "$FOCUSED" != "mcpelauncher-client" ]; then
    echo "Sending notification: Focus Minecraft first!"
    notify-send "Autoclicker" "Focus Minecraft first!" --expire-time=1000
    exit 1
fi

if [ "$ACTION" = "start" ] || [ "$ACTION" = "toggle" ]; then
    # If it's already running, don't start multiple loops
    if [ -f "$RUNFILE" ] && [ "$(cat "$RUNFILE")" = "1" ]; then
        if [ "$ACTION" = "start" ]; then
            echo "Already running, doing nothing."
            exit 0
        else
            # Toggle off
            echo "0" > "$RUNFILE"
            notify-send "Autoclicker" "$LABEL OFF 🔴" --expire-time=1000
            exit 0
        fi
    fi

    echo "1" > "$RUNFILE"
    echo "Starting new autoclicker loop..."
    notify-send "Autoclicker" "$LABEL ON 🟢" --expire-time=1000
    (
        while [ "$(cat "$RUNFILE" 2>/dev/null)" = "1" ]; do
            FOCUSED=$(hyprctl activewindow | grep "class:" | awk '{print $2}')
            if [ "$FOCUSED" = "mcpelauncher-client" ]; then
                ydotool click --next-delay 35 $BUTTON
            else
                sleep 0.5
            fi
        done
        echo "Loop exited cleanly for $LABEL."
    ) &
fi
