# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

include(CMakeFindDependencyMacro)
if (CMAKE_SYSTEM_NAME MATCHES Linux)
  # Ubuntu 20.04 symlinks /lib to /usr/lib but there is no equivalent /include folder/link.
  # This causes FindBoost to incorrectly set the headers path to non-existent /include folder.
  # Work around it by temporarily updating CMAKE_PREFIX_PATH.
  if (EXISTS "/lib" AND NOT EXISTS "/include")
    message (WARNING "Updating cmake prefix path from '${CMAKE_PREFIX_PATH}' to '/usr' temporarily for FindBoost")
    set(DO_CMAKE_PREFIX_PATH_SAVE ${CMAKE_PREFIX_PATH})
    set(CMAKE_PREFIX_PATH "/usr")
  endif ()
endif ()
if (${CMAKE_VERSION} VERSION_GREATER "3.9.0")
    find_dependency(Boost COMPONENTS filesystem system)
else ()
    # Old cmake versions do not support extra args to find_dependency
    find_dependency(Boost)
endif ()
if (DO_CMAKE_PREFIX_PATH_SAVE)
  message (STATUS "Restoring cmake prefix path to '${DO_CMAKE_PREFIX_PATH_SAVE}'")
  set(CMAKE_PREFIX_PATH ${DO_CMAKE_PREFIX_PATH_SAVE})
endif ()
include("${CMAKE_CURRENT_LIST_DIR}/deliveryoptimization_sdk-targets.cmake")
