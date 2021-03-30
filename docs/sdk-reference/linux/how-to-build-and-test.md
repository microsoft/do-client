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
> ./bootstrap-ubuntu-18.04.sh
```

### Building DO client components 
**NOTICE:**  
**If you are modifying this project and distributing your own custom build, please modify the DO_BUILDER_IDENTIFIER cmake variable located in https://github.com/microsoft/do-client/blob/main/CMakeLists.txt**

After setting up your development machine, navigate back into the project root

```markdown
> cd <project root>
```

Run this command to build the sdk and package it into a .deb file  

```markdown
> python3 build/build.py --project sdk --package-for deb  
```

### Testing DO Client components

As guidance, please ensure proper code coverage for project contributions  
Unit tests for the agent and sdk are produced as a part of the above build command, you can find them in the build output directory

```markdown
> cd /tmp/build-deliveryoptimization-agent/linux-debug/client-lite/test
```

Our tests utilize the [GTest](https://github.com/google/googletest) unit testing framework, which supports test filtering via command line  
You can run all agent tests by running

The test executable for the SDK is located in the sdk build output as well

```markdown
> cd /tmp/build-deliveryoptimization-sdk/linux-debug/sdk-cpp/tests
```

The sdk tests expect a running do-agent, you can either manually run the agent executable from its build output or install the agent package  
You can run the sdk tests just like the agent tests

```markdown
> sudo ./deliveryoptimization-sdk-tests
```

And filter them similarly

```markdown
> sudo ./deliveryoptimization-sdk-tests --gtest_filter=DownloadTests*
```