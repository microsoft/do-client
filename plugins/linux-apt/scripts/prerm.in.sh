#!/bin/bash

# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

# Exit early to fail the remove if any command here fails
set -e

echo "Running pre-removal script for @target_bin@"

echo "Restoring method"
if [ -f @apt_methods_root@/http-backup ]; then
    mv @apt_methods_root@/http-backup @apt_methods_root@/http
    echo "Restored original methods."
else
    echo "Method backup not found. Not doing anything."
fi

ls -l @apt_methods_root@/http

echo "Done!"
