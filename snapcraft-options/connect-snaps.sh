#! /bin/bash

# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

# $ snap connect <snap>:<plug> <snap>:<slot>

set -e

# Plug agent snap into sdk-tests snap downloads slot
sudo snap connect deliveryoptimization-agent:deviceupdate-agent-downloads deliveryoptimization-sdk-tests:downloads-folder

# Plug sdk-tests snap into agent's run and config slots
sudo snap connect deliveryoptimization-sdk-tests:do-port-numbers deliveryoptimization-agent:do-port-numbers
sudo snap connect deliveryoptimization-sdk-tests:do-configs deliveryoptimization-agent:do-configs
