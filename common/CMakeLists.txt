# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

if (DO_PLATFORM_LINUX OR DO_PLATFORM_MAC)
    add_subdirectory(lib-dohttp)
endif()

if (DO_BUILD_TESTS)
    # Plugins subproject doesn't have test code
    if (DO_INCLUDE_AGENT OR DO_INCLUDE_SDK)
        add_subdirectory(lib-dotestutil)
    endif()
endif()

add_subdirectory(lib-doversion)
