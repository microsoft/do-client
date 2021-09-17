include(CMakeFindDependencyMacro)
if (${CMAKE_VERSION} VERSION_GREATER "3.9.0")
    find_dependency(Boost COMPONENTS filesystem system)
else ()
    # Old cmake versions do not support extra args to find_dependency
    find_dependency(Boost)
endif ()
include("${CMAKE_CURRENT_LIST_DIR}/deliveryoptimization_sdk-targets.cmake")
