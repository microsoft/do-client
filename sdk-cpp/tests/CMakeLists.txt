## Tests for DO SDK

find_package(Boost COMPONENTS filesystem program_options REQUIRED)
find_package(GTest REQUIRED)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")

set (dosdkcpp_private_includes
    "../src/internal/rest"
    "../src/internal/util"
    "../src/internal"
)

file (GLOB test_source
    *.cpp
    rest/*.cpp
)
add_executable(deliveryoptimization-sdk-tests ${test_source})
target_include_directories(deliveryoptimization-sdk-tests PRIVATE
    "."
    "../include"
    ${dosdkcpp_private_includes}
)
target_link_libraries(deliveryoptimization-sdk-tests
    Microsoft::deliveryoptimization
    dotestutil
    stdc++fs
    ${Boost_LIBRARIES}
    GTest::GTest
)
endif() # Linux