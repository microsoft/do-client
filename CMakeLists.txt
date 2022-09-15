# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

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

# Set target OS platform
if (CMAKE_SYSTEM_NAME MATCHES Linux)
    set(DO_PLATFORM_LINUX 1)
    message (STATUS "DO Platform: Linux")
elseif (CMAKE_SYSTEM_NAME MATCHES Windows)
    set(DO_PLATFORM_WINDOWS 1)
    message (STATUS "DO Platform: Windows")
elseif (CMAKE_SYSTEM_NAME MATCHES Darwin) # Mac
    set(DO_PLATFORM_MAC 1)
    message (STATUS "DO Platform: Mac")
else()
    message(FATAL_ERROR "Unknown platform")
endif()

# Set target lib<-->client interface
if (DO_PLATFORM_LINUX OR DO_PLATFORM_MAC)
    set(DO_INTERFACE_REST 1)
    message (STATUS "DO Interface: REST")
elseif (DO_PLATFORM_WINDOWS)
    set(DO_INTERFACE_COM 1)
    message (STATUS "DO Interface: COM")
else()
    message(FATAL_ERROR "Unknown platform for interface")
endif()

# Set target client
# Note: In the future, may need to supply a variable to build.py which specifies which agent to build on linux.
# For now, linux support of the SDK is limited to only the open-sourced agent.
if (DO_PLATFORM_MAC OR DO_PLATFORM_WINDOWS)
    set(DO_CLIENT_DOSVC 1)
    message (STATUS "DO Client: DoSvc")
elseif (DO_PLATFORM_LINUX)
    set(DO_CLIENT_AGENT 1)
    message (STATUS "DO Client: Agent")
else()
    message(FATAL_ERROR "Unknown platform for client")
endif()

# PIC (Position Independent Code) ensures .a files can be linked to executables that have PIE enabled
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# TODO: msvc and gcc support different compiler/linker flags, need to go through all flags and verify what's needed for each platform
# Certain flags cause build issues because the flag itself isn't supported on msvc
# Offending CMAKE_CXX_FLAGS: -Wformat
# Offending CMAKE_EXE_LINKER_FLAGS: relro
if (DO_PLATFORM_LINUX)
    # PIE (Position Independent Executable) ensures exe/.so can run when ASLR is enabled in the target OS
    set(COMPILER_HARDENING_FLAGS
        "-fPIE -pie -D_FORTIFY_SOURCE=2 -fstack-protector-strong -Wformat -Werror=format-security -Wreturn-local-addr")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall ${COMPILER_HARDENING_FLAGS} -fmerge-all-constants")

    # relro+now thwarts some attack vectors by reordering some ELF data structures and also by making the GOT read-only
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pie -z relro -z now")
endif (DO_PLATFORM_LINUX)

if (DO_PLATFORM_WINDOWS AND DO_INCLUDE_SDK)
    # Replicate some of MSEdge build config to catch issues before integration.
    # Use Visual Studio 2019 and the x64-Clang-Debug-SDK cmake config to enable these configs.
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set(DISABLE_WARNINGS "-Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-reserved-id-macro -Wno-global-constructors -Wno-exit-time-destructors -Wno-extra-semi-stmt")
        # Note: Many warnings to fix before we can turn on -Werror
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wreturn-stack-address -Wno-string-conversion ${DISABLE_WARNINGS}")
    endif ()
    if (NOT DO_BUILD_TESTS)
        # Disable C++ exceptions
        string(REGEX REPLACE "/EH[a-z]+" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D_HAS_EXCEPTIONS=0")
        # Disable RTTI
        string(REGEX REPLACE "/GR" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /GR-")
    endif ()
endif ()

# Define DEBUG macro in debug builds
string(TOLOWER ${CMAKE_BUILD_TYPE} DO_BUILD_TYPE)
if (DO_BUILD_TYPE MATCHES debug)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")
endif()

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

if (DO_PLATFORM_LINUX)
    set(DOSVC_BIN_NAME "deliveryoptimization-agent")
    set(DO_PLUGIN_APT_BIN_NAME "deliveryoptimization-plugin-apt")
endif()

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
