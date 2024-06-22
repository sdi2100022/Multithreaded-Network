#!/bin/bash

if [ "$#" -lt 2 ]; then
    echo "Usage: $0 [serverName] [portnum]"
    exit 1
fi

serverName=$1
portnum=$2

# List of commands, with a special prefix for jobCommander commands
commands=(
    "#issueJob touch myFile.txt"
    "ls myFile.txt"
    "#issueJob rm myFile.txt"
    "ls myFile.txt"
    "#exit"
)

for cmd in "${commands[@]}"; do
    if [[ $cmd == \#* ]]; then
        # Remove the special character (#) and run jobCommander command in a new terminal
        job_cmd=${cmd:1}
        gnome-terminal -- bash -c "./bin/jobCommander $serverName $portnum $job_cmd; exec bash"
    else
        # Run regular shell command directly
        bash -c "$cmd"
    fi
done