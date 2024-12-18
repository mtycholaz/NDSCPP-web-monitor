#!/bin/bash

# Loop to repeatedly run ./tests
while ./tests; do
    echo "Test succeeded, running again..."
done

# Exit the loop when ./tests fails
echo "Test failed, exiting."

