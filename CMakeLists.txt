cmake_minimum_required(VERSION 3.7)
# The version command, to take effect, must be the first command to execute in the cmake generate step

project (do_project_root)

# Use the build/build.py script to choose and build a subproject.
# More cmake options are defined by individual subprojects.
# Only one subproject can be built with each invocation of cmake. This avoids confusion in specifying options
# exposed by an individual subproject.
option (DO_INCLUDE_AGENT "Build subproject client-lite" OFF)
option (DO_INCLUDE_PLUGINS "Build subproject plugins" OFF)
option (DO_INCLUDE_SDK "Build subproject sdk-cpp" OFF)

option (DO_BUILD_TESTS "Set DO_BUILD_TESTS to OFF to skip building tests." ON)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Get verbose output from cmake generation and build steps
set(CMAKE_VERBOSE_MAKEFILE ON)

# PIC (Position Independent Code) ensures .a files can be linked to executables that have PIE enabled
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# PIE (Position Independent Executable) ensures exe/.so can run when ASLR is enabled in the target OS
set(COMPILER_HARDENING_FLAGS
    "-fPIE -pie -D_FORTIFY_SOURCE=2 -fstack-protector-strong -Wformat -Werror=format-security")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall ${COMPILER_HARDENING_FLAGS} -fmerge-all-constants")

# relro+now thwarts some attack vectors by reordering some ELF data structures and also by making the GOT read-only
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pie -z relro -z now")

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/common/cmake)
include (do-build-helpers)
include (do-packaging-helpers)

# For contributors, please modify builder name here if changes are made to project source
set(DO_BUILDER_IDENTIFIER "DU")
message(STATUS "Using builder name: ${DO_BUILDER_IDENTIFIER}")
message(STATUS "NOTE: Please modify builder name if modifying project source")

if (DO_BUILD_TIMESTAMP)
    message(STATUS "Build timestamp found: ${DO_BUILD_TIMESTAMP}")
else ()
    # Note: This is evaluated during generate phase only. Need to generate again to refresh it.
    string(TIMESTAMP DO_BUILD_TIMESTAMP "%Y%m%d.%H%M%S" UTC)
    message (STATUS "Build timestamp NOT found. Generated it via cmake: ${DO_BUILD_TIMESTAMP}.")
endif ()

# Use ms- prefix to avoid clashes with another company's product named do-agent.
# Use it also for other sub-projects to have consistency.
set(DOSVC_BIN_NAME "deliveryoptimization-agent")
set(DO_PLUGIN_APT_BIN_NAME "deliveryoptimization-plugin-apt")

# Only one subproject can be built with each invocation of cmake. This avoids confusion in specifying options
# exposed by an individual subproject.
if (DO_INCLUDE_AGENT)
    message (STATUS "Including subproject client-lite")
    add_subdirectory(client-lite)
elseif (DO_INCLUDE_PLUGINS)
    message (STATUS "Including subproject plugins")
    add_subdirectory(plugins)
elseif (DO_INCLUDE_SDK)
    message (STATUS "Including subproject sdk-cpp")
    add_subdirectory(sdk-cpp)
else ()
    message (WARNING "No subproject chosen. Nothing is configured to be built.")
endif ()
