# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

find_package(Boost COMPONENTS program_options REQUIRED)
find_package(GTest REQUIRED)

file (GLOB files_docs_tests
    *.cpp)
add_executable(deliveryoptimization-agent-tests ${files_docs_tests})
add_platform_interface_definitions(deliveryoptimization-agent-tests)
target_link_libraries(deliveryoptimization-agent-tests
    docs_common
    dotestutil
    ${Boost_LIBRARIES}
    GTest::GTest
    ${CXX_FILESYSTEM_LIBS}
)
