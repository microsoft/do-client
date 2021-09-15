## Tests for DO SDK

find_package(Boost COMPONENTS filesystem program_options REQUIRED)
find_package(GTest REQUIRED)

set(sdk_tests_compile_definitions_common
    DO_INTERFACE_ID=${DO_INTERFACE_ID}
    DO_INTERFACE_ID_REST=${DO_INTERFACE_ID_REST}
    DO_INTERFACE_ID_COM=${DO_INTERFACE_ID_COM}
    DO_INTERFACE_ID_UNKNOWN=${DO_INTERFACE_ID_UNKNOWN}
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

    set(sdk_tests_linked_libs
        stdc++fs
        dotestutil
    )

    file(GLOB test_source
        ${test_source_common}
        "rest/*.cpp"
    )

elseif (${DO_PLATFORM_ID} EQUAL ${DO_PLATFORM_ID_MAC})

    set(dosdkcpp_private_includes
        "../src/internal/rest"
        "../src/internal/rest/util"
    )

    set(sdk_tests_linked_libs
        dotestutil
    )

    # Many of the rest interface tests test simple client specific functionality, should make a seperate folder for simple client
    file(GLOB test_source
        ${test_source_common}
        "rest/test_helpers.cpp"
    )

elseif (${DO_PLATFORM_ID} EQUAL ${DO_PLATFORM_ID_WINDOWS})

    set(dosdkcpp_private_includes
        "../src/internal/com"
        "../src/internal/com/util"
    )
    file(GLOB test_source
        ${test_source_common}
        "com/*.cpp"
    )

endif() # Windows

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