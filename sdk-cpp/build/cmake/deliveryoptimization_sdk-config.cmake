include(CMakeFindDependencyMacro)
find_dependency(Boost COMPONENTS filesystem system)
include("${CMAKE_CURRENT_LIST_DIR}/deliveryoptimization_sdk-targets.cmake")
