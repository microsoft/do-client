# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

## Tests for DO SDK nothrow interface
## Currently building for Windows only

find_package(Boost COMPONENTS filesystem program_options REQUIRED)
find_package(GTest REQUIRED)

set(sdk_tests_linked_libs_common
    Microsoft::deliveryoptimization
    ${Boost_LIBRARIES}
    GTest::GTest
)

set(dosdkcpp_private_includes_common
    "."
    "../include"
    "../src/internal"
)

set(test_source_common
    "*.cpp"
)

set(dosdkcpp_private_includes
    "../src/internal/com"
    "../src/internal/com/util"
)
file(GLOB test_source
    ${test_source_common}
    "com/*.cpp"
)

add_executable(deliveryoptimization-sdk-nothrow-tests ${test_source})
add_platform_interface_definitions(deliveryoptimization-sdk-nothrow-tests)
# DO_ENABLE_EXCEPTIONS not defined; enables nothrow interface

target_include_directories(deliveryoptimization-sdk-nothrow-tests
    PRIVATE
        ${dosdkcpp_private_includes}
        ${dosdkcpp_private_includes_common}
)

target_link_libraries(deliveryoptimization-sdk-nothrow-tests
    ${sdk_tests_linked_libs}
    ${sdk_tests_linked_libs_common}
)