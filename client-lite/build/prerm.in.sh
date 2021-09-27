#!/bin/bash

# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

svc_name=@docs_svc_name@

echo "Running pre-removal script for $svc_name"

echo "Stopping and disabling service $svc_name"
systemctl stop $svc_name
systemctl disable $svc_name

echo "Done!"
