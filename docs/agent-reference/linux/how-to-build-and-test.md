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

We provide an easy-to-use python script for building our client components from the project root, you can inspect build.py for additional build flags  
On debian-based systems, run this command to build the client and package it as a .deb file

```markdown
> python3 build/build.py --project agent --package-for deb
```

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