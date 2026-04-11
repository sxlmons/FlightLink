#!/bin/bash

COUNT=${1:-100}
IP_ADDR=$2
CLIENT_NAME="main" # The name of your executable

while true; do
    # Count how many processes with this name are currently running
    CURRENT_RUNNING=$(pgrep -c -x "$CLIENT_NAME")
    
    # Calculate how many more we need
    NEEDED=$((COUNT - CURRENT_RUNNING))
    
    if [ "$NEEDED" -gt 0 ]; then
        echo "Running: $CURRENT_RUNNING. Spawning $NEEDED more..."
        for (( i=1; i<=NEEDED; i++ )); do
            ./Client/main "$IP_ADDR" > /dev/null 2>&1 &
        done
    fi

    sleep 1 # Check once per second
done