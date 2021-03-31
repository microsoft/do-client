# Delivery Optimization Client

This repository contains source code for the following DO components:

* Agent
* SDK
* Plug-ins

## Agent

Delivery Optimization HTTP downloader with Microsoft Connected Cache support.

## SDK

Library for enabling inter-process communication (IPC) through native C++
code for Delivery Optimization Agent on Linux.

## Plug-ins

Add-on that enables APT downloads to go through Delivery Optimization Agent.
It is requires the SDK and Agent components.

## Build and Test
* [Getting Started on Linux](https://github.com/microsoft/do-client/tree/main/docs/getting-started-on-linux)

## Support

This repository is currently in a **Public Preview** state.  During this phase, all DO components
found in this repo will be supported for 90 days beyond the release date of a new release.  At
the end of the 90 day window, we will not guarantee support for the previous version.  Please plan
to migrate to the new DO components within that 90-day window to avoid any disruptions.

## Filing a Bug

Please file a [GitHub Issue](https://github.com/microsoft/do-client/issues) to ensure all issues are
tracked appropriately.

## Build status

#### Ubuntu 18.04

| Architecture | Agent | SDK | Plugin |
|-----|--------|-----|--------|
| x86-64 | [![Build Status](https://deliveryoptimization.visualstudio.com/client/_apis/build/status/DO%20Simple%20Client%20x86-64%20Build?branchName=main)](https://deliveryoptimization.visualstudio.com/client/_build/latest?definitionId=45&branchName=main) | [![Build Status](https://deliveryoptimization.visualstudio.com/client/_apis/build/status/DO%20CPP-SDK%20x86-64%20Build?branchName=main)](https://deliveryoptimization.visualstudio.com/client/_build/latest?definitionId=46&branchName=main) | [![Build Status](https://deliveryoptimization.visualstudio.com/client/_apis/build/status/DO%20Plugins%20x86-64%20Build?branchName=main)](https://deliveryoptimization.visualstudio.com/client/_build/latest?definitionId=47&branchName=main) |
| arm64 | [![Build Status](https://deliveryoptimization.visualstudio.com/client/_apis/build/status/DO%20Simple%20Client%20ARM%20Build?branchName=main)](https://deliveryoptimization.visualstudio.com/client/_build/latest?definitionId=25&branchName=main) | [![Build Status](https://deliveryoptimization.visualstudio.com/client/_apis/build/status/DO%20CPP-SDK%20ARM%20Build?branchName=main)](https://deliveryoptimization.visualstudio.com/client/_build/latest?definitionId=33&branchName=main) | [![Build Status](https://deliveryoptimization.visualstudio.com/client/_apis/build/status/DO%20Plugins%20ARM%20Build?branchName=main)](https://deliveryoptimization.visualstudio.com/client/_build/latest?definitionId=31&branchName=main) |

#### Debian 9

| Architecture | Agent | SDK | Plugin |
|-----|--------|-----|--------|
| arm32 | [![Build Status](https://deliveryoptimization.visualstudio.com/client/_apis/build/status/DO%20Simple%20Client%20ARM%20Build?branchName=main)](https://deliveryoptimization.visualstudio.com/client/_build/latest?definitionId=25&branchName=main) | [![Build Status](https://deliveryoptimization.visualstudio.com/client/_apis/build/status/DO%20CPP-SDK%20ARM%20Build?branchName=main)](https://deliveryoptimization.visualstudio.com/client/_build/latest?definitionId=33&branchName=main) | [![Build Status](https://deliveryoptimization.visualstudio.com/client/_apis/build/status/DO%20Plugins%20ARM%20Build?branchName=main)](https://deliveryoptimization.visualstudio.com/client/_build/latest?definitionId=31&branchName=main) |

#### Debian 10

| Architecture | Agent | SDK | Plugin |
|-----|--------|-----|--------|
| arm32 | [![Build Status](https://deliveryoptimization.visualstudio.com/client/_apis/build/status/DO%20Simple%20Client%20ARM%20Build?branchName=main)](https://deliveryoptimization.visualstudio.com/client/_build/latest?definitionId=25&branchName=main) | [![Build Status](https://deliveryoptimization.visualstudio.com/client/_apis/build/status/DO%20CPP-SDK%20ARM%20Build?branchName=main)](https://deliveryoptimization.visualstudio.com/client/_build/latest?definitionId=33&branchName=main) | [![Build Status](https://deliveryoptimization.visualstudio.com/client/_apis/build/status/DO%20Plugins%20ARM%20Build?branchName=main)](https://deliveryoptimization.visualstudio.com/client/_build/latest?definitionId=31&branchName=main) |

## Contact

Directly contact us: <docloss@microsoft.com>
