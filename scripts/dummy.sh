#!/bin/bash

function handle_int {
    echo "Received SIGINT or SIGTERM, keep running for 15 seconds nevertheless"
    sleep 15
    exit -5
}
trap "handle_int" SIGINT SIGTERM

while [[ true ]]; do
    echo $RANDOM
    sleep 1
done
