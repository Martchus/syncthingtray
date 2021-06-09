#!/bin/bash

function handle_int {
    echo "Received SIGINT or SIGTERM, keep running for 15 seconds nevertheless"
    sleep 15
    exit -5
}
trap "handle_int" SIGINT SIGTERM

i=0
while [[ true ]]; do
    echo "$i : $RANDOM"
    i=$((i + 1))
    sleep 1
done
