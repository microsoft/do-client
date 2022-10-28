# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

set(test_lib_name dotestutil)

find_package(Boost COMPONENTS system)   # for asio

add_library(${test_lib_name} STATIC do_test_helpers.cpp)
add_platform_interface_definitions(${test_lib_name})
target_include_directories(${test_lib_name}
    PUBLIC
        ${Boost_INCLUDE_DIRS} #boost/asio.hpp
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)