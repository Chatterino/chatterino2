#!/usr/bin/env bash

set -e

# This is a script that runs then kills chatterino at random intervals
# You can modify the _min_ms and _rand_ms to change how often they're killed

_min_ms="1000"
_rand_ms="8000"

if [ -z "$1" ]; then
    echo "usage: $0 <path_to_chatterino_bin_to_run> (e.g. $0 ./build-mold/bin/chatterino)"
    exit
fi

bin_to_run="$1"

while true; do
    sleep_time_ms=$(( RANDOM % _rand_ms + _min_ms ))
    sleep_time="$(echo "${sleep_time_ms}/1000" | bc)"

    $bin_to_run &
    pid=$!

    echo ""
    echo ""
    echo ""
    echo ""
    echo ""
    echo ""
    echo ""
    echo ""
    echo ">>>>>>> SLEEPING FOR ${sleep_time}s"

    (sleep "$sleep_time" && echo "" && echo "<<<<<<<<<<<<< TRY KILL" && kill -SIGINT "$pid") &

    wait "$pid"

    sleep 0.5
done
