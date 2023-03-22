# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

macro(override_prefix_path_if_needed)
    if (CMAKE_SYSTEM_NAME MATCHES Linux)
        # Ubuntu 20.04 symlinks /lib to /usr/lib but there is no equivalent /include path.
        # This causes FindBoost to incorrectly set the headers path to the non-existent /include path.
        # Work around it by temporarily updating CMAKE_PREFIX_PATH.
        if (EXISTS "/lib" AND NOT EXISTS "/include")
            if (CMAKE_PREFIX_PATH)
                message (WARNING "/lib exists but not /include, and CMAKE_PREFIX_PATH is already set. FindBoost may result in non-existent include dir.")
            else ()
                message (WARNING "Updating cmake prefix path from '${CMAKE_PREFIX_PATH}' to '/usr' temporarily for FindBoost.")
                set(DO_CMAKE_PREFIX_PATH_SAVE ${CMAKE_PREFIX_PATH})
                set(CMAKE_PREFIX_PATH "/usr")
            endif ()
        endif ()
    endif ()
endmacro()

macro(restore_prefix_path_if_needed)
    if (DO_CMAKE_PREFIX_PATH_SAVE)
        message (STATUS "Restoring cmake prefix path to '${DO_CMAKE_PREFIX_PATH_SAVE}'")
        set(CMAKE_PREFIX_PATH ${DO_CMAKE_PREFIX_PATH_SAVE})
        # Cleanup temporary variable
        set(DO_CMAKE_PREFIX_PATH_SAVE)
    endif ()
endmacro()

macro(fixup_do_includes_if_needed)
    if (CMAKE_SYSTEM_NAME MATCHES Linux)
        # See explanation in macro override_prefix_path_if_needed, above.
        # Setting CMAKE_PREFIX_PATH does not work for DO SDK though. So update the INTERFACE_INCLUDE_DIRECTORIES property directly.
        get_target_property(DOSDK_INC_DIR_PRIV Microsoft::deliveryoptimization INTERFACE_INCLUDE_DIRECTORIES)
        if (${DOSDK_INC_DIR_PRIV} MATCHES "^/include" AND NOT EXISTS "/include")
            message(WARNING "/include does not exist. Updating include dirs from ${DOSDK_INC_DIR_PRIV} to /usr${DOSDK_INC_DIR_PRIV}.")
            set_target_properties(Microsoft::deliveryoptimization PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "/usr${DOSDK_INC_DIR_PRIV}")
        endif ()
        # Cleanup temporary variable
        set(DOSDK_INC_DIR_PRIV)
    endif ()
endmacro()

# ** main **

if (TARGET Microsoft::deliveryoptimization)
    return()
endif ()

include(CMakeFindDependencyMacro)

override_prefix_path_if_needed()
if (${CMAKE_VERSION} VERSION_GREATER "3.9.0")
    find_dependency(Boost COMPONENTS filesystem system)
else ()
    # Old cmake versions do not support extra args to find_dependency
    find_dependency(Boost)
endif ()
restore_prefix_path_if_needed()

include("${CMAKE_CURRENT_LIST_DIR}/deliveryoptimization_sdk-targets.cmake")

if (TARGET Microsoft::deliveryoptimization)
    fixup_do_includes_if_needed()
endif ()
