# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

## Tests for DO SDK

find_package(Boost COMPONENTS filesystem program_options REQUIRED)
find_package(GTest REQUIRED)

set(sdk_tests_linked_libs_common
    Microsoft::deliveryoptimization
    ${Boost_LIBRARIES}
    GTest::GTest
    dotestutil
)

set(dosdkcpp_private_includes_common
    "."
    "../include"
    "../src/internal"
)

set(test_source_common
    "*.cpp"
)

if(DO_PLATFORM_LINUX)

    set(dosdkcpp_private_includes
        "../src/internal/rest"
        "../src/internal/rest/util"
    )

    set(sdk_tests_linked_libs
        stdc++fs
    )

    set(test_source_private
        "rest/*.cpp"
    )

elseif (DO_PLATFORM_MAC)

    set(dosdkcpp_private_includes
        "../src/internal/rest"
        "../src/internal/rest/util"
    )

    # Many of the rest interface tests test simple client specific functionality, should make a seperate folder for simple client
    set(test_source_private
        "rest/test_helpers.cpp"
    )

elseif(DO_PLATFORM_WINDOWS)

    set(dosdkcpp_private_includes
        "../src/internal/com"
        "../src/internal/com/util"
    )

endif() # Windows

file (GLOB test_source
    ${test_source_common}
    ${test_source_private}
)

add_executable(deliveryoptimization-sdk-tests ${test_source})
add_platform_interface_definitions(deliveryoptimization-sdk-tests)

target_include_directories(deliveryoptimization-sdk-tests
    PRIVATE
        ${dosdkcpp_private_includes}
        ${dosdkcpp_private_includes_common}
)

target_link_libraries(deliveryoptimization-sdk-tests
    ${sdk_tests_linked_libs}
    ${sdk_tests_linked_libs_common}
)