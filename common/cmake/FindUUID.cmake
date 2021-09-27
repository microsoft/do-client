# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

cmake_minimum_required (VERSION 3.7)

# cmake find module for the UUID linux library

include(FindPackageHandleStandardArgs)

find_path(
    UUID_INCLUDE_DIR
    NAMES uuid.h
    PATH_SUFFIXES uuid
)
find_library(UUID_LIBRARY uuid)

find_package_handle_standard_args(
    UUID
    DEFAULT_MSG
    UUID_INCLUDE_DIR
    UUID_LIBRARY
)

if(UUID_FOUND)
    set(UUID_INCLUDE_DIRS ${UUID_INCLUDE_DIR})
    set(UUID_LIBRARIES ${UUID_LIBRARY})

    if(NOT TARGET UUID::UUID)
        add_library(UUID::UUID INTERFACE IMPORTED)
        set_target_properties(
            UUID::UUID
            PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${UUID_INCLUDE_DIR}"
                       INTERFACE_LINK_LIBRARIES "${UUID_LIBRARIES}"
        )
    endif()
endif()

mark_as_advanced(UUID_INCLUDE_DIR UUID_LIBRARY)
