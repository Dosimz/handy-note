#!/bin/bash

while true; do
    gtimeout 10 git push origin main
    if [ $? -eq 0 ]; then
        echo "Git push successful!"
        break
    else
        echo "Git push failed or timed out, retrying in 10 seconds..."
        sleep 10
    fi
done
