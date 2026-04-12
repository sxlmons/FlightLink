#!/bin/bash

COUNT=${1:-100}
IP_ADDR=$2
BATCH_SIZE=${3:-50}
CLIENT_NAME="Client"

while true; do
    CURRENT_RUNNING=$(pgrep -x "$CLIENT_NAME" | wc -l | tr -d ' ')
    NEEDED=$((COUNT - CURRENT_RUNNING))

    if [ "$NEEDED" -gt 0 ]; then

        if [ "$NEEDED" -gt "$BATCH_SIZE" ]; then
            SPAWN=$BATCH_SIZE
        else
            SPAWN=$NEEDED
        fi

        echo "Running: $CURRENT_RUNNING. Spawning $SPAWN more..."
        for (( i=1; i<=SPAWN; i++ )); do
            ./Client/build/Client "$IP_ADDR" > /dev/null 2>&1 &
        done
    fi

    sleep 1
done
