# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

if (NOT TARGET Microsoft::deliveryoptimization)
    include("${CMAKE_CURRENT_LIST_DIR}/deliveryoptimization_sdk-targets.cmake")
endif ()

if (TARGET Microsoft::deliveryoptimization)
    if (CMAKE_SYSTEM_NAME MATCHES Linux)

        # Ubuntu 20.04 symlinks /lib to /usr/lib but there is no equivalent /include path.
        # This makes cmake's auto-generated targets file set include dirs to the non-existent
        # /include path. Fix it by updating the INTERFACE_INCLUDE_DIRECTORIES property directly.

        get_target_property(DOSDK_INC_DIR_PRIV Microsoft::deliveryoptimization INTERFACE_INCLUDE_DIRECTORIES)
        if (${DOSDK_INC_DIR_PRIV} MATCHES "^/include" AND NOT EXISTS "/include")
            message(WARNING "/include does not exist. Updating include dirs from ${DOSDK_INC_DIR_PRIV} to /usr${DOSDK_INC_DIR_PRIV}.")
            set_target_properties(Microsoft::deliveryoptimization PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "/usr${DOSDK_INC_DIR_PRIV}")
        endif ()

        # Cleanup temporary variable
        set(DOSDK_INC_DIR_PRIV)

    endif ()
endif ()
