#!/bin/bash

if [ "$#" -lt 2 ]; then
    echo "Usage: $0 [serverName] [portNum]"
    exit 1
fi

serverName=$1
portNum=$2
bufferSize=10
threadPoolSize=10

test_dir="./tests"

for i in {1..8}; do
    gnome-terminal -- bash -c "./bin/jobExecutorServer $portNum $bufferSize $threadPoolSize; exec bash"
    script="$test_dir/test_jobExecutor_$i.sh"
    if [[ -f $script ]]; then
        echo "Running $script with arguments $serverName $portNum..."
        bash "$script" "$serverName" "$portNum"
    else
        echo "Script $script not found!"
    fi
done