## Tests for DO SDK

find_package(Boost COMPONENTS filesystem program_options REQUIRED)
find_package(GTest REQUIRED)

set(sdk_tests_compile_definitions_common
        DO_PLATFORM_ID=${DO_PLATFORM_ID}
        DO_PLATFORM_ID_UNKNOWN=${DO_PLATFORM_ID_UNKNOWN}
        DO_PLATFORM_ID_LINUX=${DO_PLATFORM_ID_LINUX}
        DO_PLATFORM_ID_MAC=${DO_PLATFORM_ID_MAC}
        DO_PLATFORM_ID_WINDOWS=${DO_PLATFORM_ID_WINDOWS}
)
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

if(${DO_PLATFORM_ID} EQUAL ${DO_PLATFORM_ID_LINUX})

set(dosdkcpp_private_includes
    "../src/internal/rest"
    "../src/internal/rest/util"
)

# stdc++fs required on linux due to C++14
set(sdk_tests_linked_libs
    stdc++fs
    dotestutil
)

file(GLOB test_source
    ${test_source_common}
    "rest/*.cpp"
)

endif() # Linux

add_executable(deliveryoptimization-sdk-tests ${test_source})

target_compile_definitions(deliveryoptimization-sdk-tests
    PRIVATE
        ${sdk_tests_compile_definitions_common}
)

target_include_directories(deliveryoptimization-sdk-tests
    PRIVATE
        ${dosdkcpp_private_includes}
        ${dosdkcpp_private_includes_common}
)

target_link_libraries(deliveryoptimization-sdk-tests
    ${sdk_tests_linked_libs}
    ${sdk_tests_linked_libs_common}
)