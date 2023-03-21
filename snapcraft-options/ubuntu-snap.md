# Ubuntu Snap packages - What is it, how to build, and how to test

## What
https://snapcraft.io/docs

## Building
Run the commands in the root of the repo.

- First time build or after modifying corresponding yaml file:
    - `$ ./build/build-snaps.sh agent`
    - `$ ./build/build-snaps.sh sdk-tests`

- By default snapcraft doesn't allow multiple snaps definition in the same code base, so this build-snaps.sh file was created so that we can copy the different snap files into the main snapcraft.yaml file and then build it.

- Subsequent builds of the same component without modifying the corresponding yaml file:
    - `$ snapcraft`

- The build will generate *.snap files in the current working directory. Example: **deliveryoptimization-sdk-tests_0.1_amd64.snap**.

- In the snapcraft.yaml the regular script to build for both the delivery optimization agent and sdk is called (`build.py`), but since there are some different behaviors the code should have when inside a snap, for that we use a flag `--build-for-snap` on the build. Below you can see the usage of this flag, but **is not** necessary to execute them, those lines will be executed inside the snapcraft.yaml file when you call the `build-snaps.sh` file.
    - `$ python3 ./build/build.py --project agent --build-for-snap`  
    - `$ python3 ./build/build.py --project sdk --build-for-snap`  

## Installing
- `$ sudo snap install --devmode ./deliveryoptimization-sdk-tests_0.1_amd64.snap`
- `$ sudo snap install --devmode ./deliveryoptimization-client_0.1_amd64.snap`

## The snap environment
It is a fruitful exercise to look around in the host file system and see how snap structures the installed snaps. Look at these paths to start with:
- /snap/<snap-name>
- /var/snap/<snap-name>

## Connecting to Other Snaps

- Snapcraft allows communication between different snaps by using a content interface to handle connections with the purpose share code and data between them. These connections are composed by plugs and slots, where a slot is the producer snap and the plug is the consumer snap.

- For the SDK test snap to work, the plugs and slots must be connected. Run the **connect-snaps.sh** script after both snaps are installed.
- The connections can be listed using this command:
    - `$ snap connections | grep delivery`

```shell
Interface     Plug                                                      Slot                                         Notes
content       -                                                         deliveryoptimization-client:do-configs       -
content       -                                                         deliveryoptimization-client:do-port-numbers  -
content       deliveryoptimization-client:deviceupdate-agent-downloads  -                                            -
network       deliveryoptimization-client:network                       :network                                     -
network-bind  deliveryoptimization-client:network-bind                  :network-bind                                -

```

## Running or executing
- Agent
    - The agent is declared as a daemon/service, so it starts running immediately after successful installation.
    - Stop service: `$ sudo snap stop deliveryoptimization-client.agent`
    - Start service: `$ sudo snap start deliveryoptimization-client.agent`
    - Read service journal: `$ sudo snap logs deliveryoptimization-client.agent`
    - systemctl can also be used with the correct service unit name: `$ systemctl status snap.deliveryoptimization-client.agent.service`
    - **journalctl** can also be used to follow logs: `$ journalctl -f -u snap.deliveryoptimization-client.agent.service`

- SDK tests
    - This snap declares the **deliveryoptimization-sdk-tests** binary as an app.
    - One way to run the tests is: `$ sudo snap run deliveryoptimization-sdk-tests.sdk-tests --gtest_filter=*SimpleDownloadTest`
    - Advanced  usage: get a shell into the snap and invoke the binary directly. This also allows us to inspect the runtime environment
        more easily, like listing environment variables, listing files/dirs, etc.<br>
        Use `$ sudo snap run --shell deliveryoptimization-sdk-tests.sdk-tests` to open a shell into the snap.<br>
        Then we can use regular shell commands like `printenv | grep SNAP`, `cd $SNAP`, `ls -l $SNAP_DATA/do-port-numbers/`<br>
        We can also run the test binary like so: `cd $SNAP` and then `bin/deliveryoptimization-sdk-tests --gtest_filter=*SimpleDownloadTest`

## Uninstalling or cleaning up
- Uninstall installed snap packages
    - `$ sudo snap remove deliveryoptimization-client`
    - `$ sudo snap remove deliveryoptimization-sdk-tests`

- Cleaning up build environment: Building sometimes fails with strange errors from **multipass**. Cleaning up the multipass instance helps.
    - List the available instances: `$ multipass list`
    - Delete an instance, example: `$ multipass delete snapcraft-deliveryoptimization-client`
    - Purge deleted instances: `$ multipass purge`
