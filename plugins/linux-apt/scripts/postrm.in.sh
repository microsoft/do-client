#!/bin/bash

# Exit early to fail the remove if any command here fails
set -e

echo "Running post-removal script for @target_bin@"

if [ -d @dopapt_cache_path@ ]; then
    echo "Removing @dopapt_cache_path@."
    rm -rf @dopapt_cache_path@
fi

if [ -f @dopapt_log_path@ ]; then
    echo "Removing @dopapt_log_path@."
    rm @dopapt_log_path@
fi

echo "Done!"
