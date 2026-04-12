#!/bin/bash
COUNT=${1:-100}
IP_ADDR=$2
BATCH_SIZE=${3:-50}

LAUNCHED=0
while [ "$LAUNCHED" -lt "$COUNT" ]; do
    REMAINING=$((COUNT - LAUNCHED))
    if [ "$REMAINING" -gt "$BATCH_SIZE" ]; then
        SPAWN=$BATCH_SIZE
    else
        SPAWN=$REMAINING
    fi

    for (( i=1; i<=SPAWN; i++ )); do
        ./Client/build/Client "$IP_ADDR" > /dev/null 2>&1 &
    done

    LAUNCHED=$((LAUNCHED + SPAWN))
    echo "Launched $LAUNCHED / $COUNT"

    if [ "$LAUNCHED" -lt "$COUNT" ]; then
        sleep 1
    fi
done

echo "All $COUNT clients launched."