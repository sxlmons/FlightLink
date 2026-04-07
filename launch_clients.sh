#!/bin/bash
# launch_clients.sh
# Launches 100 client instances for load/endurance testing.
#
# Usage: ./launch_clients.sh
#        ./launch_clients.sh 50        (custom count)

COUNT=${1:-100}
CLIENT_PATH="./build/Client/Client"

if [ ! -f "$CLIENT_PATH" ]; then
    echo "Error: Client not found at: $CLIENT_PATH"
    exit 1
fi

echo "Launching $COUNT clients..."

for (( i=1; i<=COUNT; i++ )); do
    $CLIENT_PATH &
    echo "  Started client $i / $COUNT"
done

echo "All $COUNT clients launched."