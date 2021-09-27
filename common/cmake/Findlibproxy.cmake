# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

cmake_minimum_required (VERSION 3.7)

# cmake find module for the libproxy library and headers

include(FindPackageHandleStandardArgs)

find_path(
    libproxy_INCLUDE_DIR
    NAMES proxy.h
    PATH_SUFFIXES include
)

find_library(libproxy_LIBRARY proxy)

find_package_handle_standard_args(
    libproxy
    DEFAULT_MSG
    libproxy_INCLUDE_DIR
    libproxy_LIBRARY
)

if(libproxy_FOUND)
    set(libproxy_INCLUDE_DIRS ${libproxy_INCLUDE_DIR})
    set(libproxy_LIBRARIES ${libproxy_LIBRARY})

    message(STATUS "lib: ${libproxy_LIBRARY}, found: ${libproxy_FOUND}")

    if(NOT TARGET libproxy::proxy)
        add_library (libproxy::proxy INTERFACE IMPORTED)
        set_target_properties(
            libproxy::proxy
            PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${libproxy_INCLUDE_DIRS}"
                       INTERFACE_LINK_LIBRARIES "${libproxy_LIBRARIES}"
        )
    endif()
endif()
