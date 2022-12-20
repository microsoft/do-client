# Delivery Optimization Client

This repository contains source code for the following DO components:

* Agent
* SDK
* Plug-ins

## Agent

Delivery Optimization HTTP downloader with Microsoft Connected Cache support.

## SDK

Library for enabling inter-process communication (IPC) with deliveryoptimization clients
through native C++ code.

## Plug-ins

Add-on that enables APT downloads to go through Delivery Optimization Agent.
It is a required component only on devices that must download APT packages via a Microsoft Connected Cache instance.
During install, it replaces itself as APT's HTTP(S) transport mechanism, thus receiving all APT downloads requests.

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

### Building DO client components
**NOTICE:**
**If you are modifying this project and distributing your own custom build, please modify the DO_BUILDER_IDENTIFIER cmake variable located in https://github.com/microsoft/do-client/blob/main/CMakeLists.txt**

After setting up your development machine, navigate back into the project root

```markdown
> cd <project root>
```

We provide an easy-to-use python script for building our client components from the project root, you can inspect build.py for additional build flags
On debian-based systems, run this command to build the client and package it as a .deb file

```markdown
> python3 build/build.py --project agent --package-for deb
```

Run this command to build the sdk

```markdown
> python3 build/build.py --project sdk --package-for deb
```

In order to build the plugin, you must build & install the sdk, an easy way to do this is to install the the packages you produced in the previous two steps

Navigate to the build output directory for the agent and install the agent package

```markdown
> cd /tmp/build-deliveryoptimization-agent/linux-debug/
> sudo apt-get install ./deliveryoptimization-agent*.deb
```

The sdk produces a runtime and development package, in this case you'll want to install both
Navigate to build output directory for the sdk and install both packages

```markdown
> cd /tmp/build-deliveryoptimization-sdk/linux-debug/
> sudo apt-get install ./libdeliveryoptimization*.deb
```

With the sdk installed, you can now build the plugin by navigating back to the project root

```markdown
> cd <project root>
> python3 build/build.py --project plugin-apt --package-for deb
```

At this point, you should have built and packaged all components

### Installing DO Client components

There are a couple ways for you to install the DO client components

1. If you have built the component into a debian package, you can simply find the debian package and install like detailed above.
This will handle installing to the appropriate paths, and also the necessary setup of DO user/group permissions needed for DO-agent.

```markdown
> cd /tmp/build-deliveryoptimization-sdk/linux-debug/
> sudo apt-get install ./libdeliveryoptimization*.deb
> cd /tmp/build-deliveryoptimization-agent/linux-debug/
> sudo apt-get install ./deliveryoptimization-agent*.deb
> cd /tmp/build-deliveryoptimization-plugin-apt/linux-debug/
> sudo apt get install ./deliveryoptimization-plugin-apt*.deb
```

2. If you build and install using cmake, or through some other custom means, be sure to setup the DO user/groups correctly in your installation.
You can reference this [script](https://github.com/microsoft/do-client/blob/main/client-lite/build/postinst.in.sh) to see how to setup the DO user/group and install DO as a daemon.

### Testing DO Client components

As guidance, please ensure proper code coverage for project contributions
Unit tests for the agent and sdk are produced as a part of the above build command, you can find them in the build output directory

```markdown
> cd /tmp/build-deliveryoptimization-agent/linux-debug/client-lite/test
```

Our tests utilize the [GTest](https://github.com/google/googletest) unit testing framework, which supports test filtering via command line
You can run all agent tests by running

```markdown
> ./deliveryoptimization-agent-tests
```

You can filter for specific tests as well, reference the GTest documentation for filtering rules and syntax
```markdown
> sudo ./deliveryoptimization-agent-tests --gtest_filter=DownloadManagerTests*
```

The test executable for the SDK is located in the sdk build output as well

```markdown
> cd /tmp/build-deliveryoptimization-sdk/linux-debug/sdk-cpp/tests
```

The sdk tests expect a running do-agent, you can either manually run the agent executable from its build output or install the agent package as you may have done while building the plugin
You can run the sdk tests just like the agent tests

```markdown
> sudo ./deliveryoptimization-sdk-tests
```

And filter them similarly

```markdown
> sudo ./deliveryoptimization-sdk-tests --gtest_filter=DownloadTests*
```

## Support

The APT plugin component is currently in a **Public Preview** state.  During this phase, it will be
supported for 90 days beyond the release date of a new release.  At the end of the 90 day window,
we will not guarantee support for the previous version.  Please plan to migrate to a newer release
within that 90-day window to avoid any disruptions.

## Filing a Bug

Please file a [GitHub Issue](https://github.com/microsoft/do-client/issues) to ensure all issues are
tracked appropriately.

## Build status

#### Ubuntu 18.04

| Architecture | Agent | SDK | Plugin |
|-----|--------|-----|--------|
| x86-64 | [![Build Status](https://deliveryoptimization.visualstudio.com/client/_apis/build/status/DO%20Simple%20Client%20x86-64%20Build?branchName=develop)](https://deliveryoptimization.visualstudio.com/client/_build/latest?definitionId=45&branchName=develop) | [![Build Status](https://deliveryoptimization.visualstudio.com/client/_apis/build/status/DO%20CPP-SDK%20x86-64%20Build?branchName=develop)](https://deliveryoptimization.visualstudio.com/client/_build/latest?definitionId=46&branchName=develop) | [![Build Status](https://deliveryoptimization.visualstudio.com/client/_apis/build/status/DO%20Plugins%20x86-64%20Build?branchName=develop)](https://deliveryoptimization.visualstudio.com/client/_build/latest?definitionId=47&branchName=develop) |
| arm64 | [![Build Status](https://deliveryoptimization.visualstudio.com/client/_apis/build/status/DO%20Simple%20Client%20ARM%20Build?branchName=develop)](https://deliveryoptimization.visualstudio.com/client/_build/latest?definitionId=25&branchName=develop) | [![Build Status](https://deliveryoptimization.visualstudio.com/client/_apis/build/status/DO%20CPP-SDK%20ARM%20Build?branchName=develop)](https://deliveryoptimization.visualstudio.com/client/_build/latest?definitionId=33&branchName=develop) | [![Build Status](https://deliveryoptimization.visualstudio.com/client/_apis/build/status/DO%20Plugins%20ARM%20Build?branchName=develop)](https://deliveryoptimization.visualstudio.com/client/_build/latest?definitionId=31&branchName=develop) |

#### Ubuntu 20.04

| Architecture | Agent | SDK | Plugin |
|-----|--------|-----|--------|
| x86-64 | [![Build Status](https://deliveryoptimization.visualstudio.com/client/_apis/build/status/DO%20Simple%20Client%20ARM%20Build?branchName=develop)](https://deliveryoptimization.visualstudio.com/client/_build/latest?definitionId=25&branchName=develop) | [![Build Status](https://deliveryoptimization.visualstudio.com/client/_apis/build/status/DO%20CPP-SDK%20ARM%20Build?branchName=develop)](https://deliveryoptimization.visualstudio.com/client/_build/latest?definitionId=33&branchName=develop) | [![Build Status](https://deliveryoptimization.visualstudio.com/client/_apis/build/status/DO%20Plugins%20ARM%20Build?branchName=develop)](https://deliveryoptimization.visualstudio.com/client/_build/latest?definitionId=31&branchName=develop) |
| arm64 | [![Build Status](https://deliveryoptimization.visualstudio.com/client/_apis/build/status/DO%20Simple%20Client%20ARM%20Build?branchName=develop)](https://deliveryoptimization.visualstudio.com/client/_build/latest?definitionId=25&branchName=develop) | [![Build Status](https://deliveryoptimization.visualstudio.com/client/_apis/build/status/DO%20CPP-SDK%20ARM%20Build?branchName=develop)](https://deliveryoptimization.visualstudio.com/client/_build/latest?definitionId=33&branchName=develop) | [![Build Status](https://deliveryoptimization.visualstudio.com/client/_apis/build/status/DO%20Plugins%20ARM%20Build?branchName=develop)](https://deliveryoptimization.visualstudio.com/client/_build/latest?definitionId=31&branchName=develop) |

#### Debian 10

| Architecture | Agent | SDK | Plugin |
|-----|--------|-----|--------|
| arm32 | [![Build Status](https://deliveryoptimization.visualstudio.com/client/_apis/build/status/DO%20Simple%20Client%20ARM%20Build?branchName=develop)](https://deliveryoptimization.visualstudio.com/client/_build/latest?definitionId=25&branchName=develop) | [![Build Status](https://deliveryoptimization.visualstudio.com/client/_apis/build/status/DO%20CPP-SDK%20ARM%20Build?branchName=develop)](https://deliveryoptimization.visualstudio.com/client/_build/latest?definitionId=33&branchName=develop) | [![Build Status](https://deliveryoptimization.visualstudio.com/client/_apis/build/status/DO%20Plugins%20ARM%20Build?branchName=develop)](https://deliveryoptimization.visualstudio.com/client/_build/latest?definitionId=31&branchName=develop) |

### Windows 10/11

| Architecture | SDK |
|-----|--------|
| x86-64 | [![Build Status](https://deliveryoptimization.visualstudio.com/client/_apis/build/status/DO%20CPP-SDK%20Windows%2010%20x64?branchName=develop)](https://deliveryoptimization.visualstudio.com/client/_build/latest?definitionId=59&branchName=develop) |

### MacOS

| Architecture | SDK |
|-----|--------|
| x86-64 | [![Build Status](https://deliveryoptimization.visualstudio.com/client/_apis/build/status/DO%20CPP-SDK%20MacOS%20X64?branchName=develop)](https://deliveryoptimization.visualstudio.com/client/_build/latest?definitionId=60&branchName=develop) |

## Contact

Directly contact us: <docloss@microsoft.com>
