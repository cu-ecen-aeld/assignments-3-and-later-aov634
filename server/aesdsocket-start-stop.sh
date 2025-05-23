#!/bin/sh

DAEMON="$(realpath "$(dirname "$0")/aesdsocket")"
DAEMON_NAME="aesdsocket"


case "$1" in
    start)
        echo "Starting $DAEMON_NAME..."
        start-stop-daemon -S -n "$DAEMON_NAME" -a "$DAEMON" -- -d
        ;;
    stop)
        echo "Stopping $DAEMON_NAME..."
        start-stop-daemon -K -n "$DAEMON_NAME"
        ;;
    restart)
        $0 stop
        $0 start
        ;;
    *)
        echo "Usage: $0 {start|stop}"
        exit 1
        ;;
esac

exit 0
