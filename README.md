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

## Getting Started

Follow the development machine setup on each desktop you'd like to use.

### Development Machine Setup

Clone the repository locally from terminal:

```markdown
> cd (to working directory of your choosing)
> git clone https://github.com/microsoft/do-client
```

Run the appropriate bootstrapper depending on development machine platform:

```markdown
> cd build/bootstrap
```

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
| x86-64 | [![Build Status](https://deliveryoptimization.visualstudio.com/client/_apis/build/status/DO%20Simple%20Client%20Ubuntu%2018.04%20x86-64?branchName=main)](https://deliveryoptimization.visualstudio.com/client/_build/latest?definitionId=23&branchName=main) | [![Build Status](https://deliveryoptimization.visualstudio.com/client/_apis/build/status/DO%20CPP-SDK%20Ubuntu%2018.04%20x86-64?branchName=main)](https://deliveryoptimization.visualstudio.com/client/_build/latest?definitionId=26&branchName=main) | [![Build Status](https://deliveryoptimization.visualstudio.com/client/_apis/build/status/DO%20Plugins%20APT%20Ubuntu%2018.04%20x86-64?branchName=main)](https://deliveryoptimization.visualstudio.com/client/_build/latest?definitionId=29&branchName=main) |
| arm64 | [![Build Status](https://deliveryoptimization.visualstudio.com/client/_apis/build/status/DO%20Simple%20Client%20Ubuntu%2018.04%20arm64?branchName=main)](https://deliveryoptimization.visualstudio.com/client/_build/latest?definitionId=37&branchName=main) | [![Build Status](https://deliveryoptimization.visualstudio.com/client/_apis/build/status/DO%20CPP-SDK%20Ubuntu%2018.04%20arm64?branchName=main)](https://deliveryoptimization.visualstudio.com/client/_build/latest?definitionId=38&branchName=main) | [![Build Status](https://deliveryoptimization.visualstudio.com/client/_apis/build/status/DO%20Plugins%20APT%20Ubuntu%2018.04%20arm64?branchName=main)](https://deliveryoptimization.visualstudio.com/client/_build/latest?definitionId=39&branchName=main) |

#### Debian 9

| Architecture | Agent | SDK | Plugin |
|-----|--------|-----|--------|
| arm32 | [![Build Status](https://deliveryoptimization.visualstudio.com/client/_apis/build/status/DO%20Simple%20Client%20Debian9%20arm32?branchName=main)](https://deliveryoptimization.visualstudio.com/client/_build/latest?definitionId=25&branchName=main) | [![Build Status](https://deliveryoptimization.visualstudio.com/client/_apis/build/status/DO%20CPP-SDK%20Debian9%20arm32?branchName=main)](https://deliveryoptimization.visualstudio.com/client/_build/latest?definitionId=33&branchName=main) | [![Build Status](https://deliveryoptimization.visualstudio.com/client/_apis/build/status/DO%20Plugins%20APT%20Debian9%20arm32?branchName=main)](https://deliveryoptimization.visualstudio.com/client/_build/latest?definitionId=31&branchName=main) |

#### Debian 10

| Architecture | Agent | SDK | Plugin |
|-----|--------|-----|--------|
| arm32 | [![Build Status](https://deliveryoptimization.visualstudio.com/client/_apis/build/status/DO%20Simple%20Client%20Debian10%20arm32?branchName=main)](https://deliveryoptimization.visualstudio.com/client/_build/latest?definitionId=24&branchName=main) | [![Build Status](https://deliveryoptimization.visualstudio.com/client/_apis/build/status/DO%20CPP-SDK%20Debian10%20arm32?branchName=main)](https://deliveryoptimization.visualstudio.com/client/_build/latest?definitionId=32&branchName=main) | [![Build Status](https://deliveryoptimization.visualstudio.com/client/_apis/build/status/DO%20Plugins%20APT%20Debian10%20arm32?branchName=main)](https://deliveryoptimization.visualstudio.com/client/_build/latest?definitionId=34&branchName=main) |

## Contact

Directly contact us: <docloss@microsoft.com>
