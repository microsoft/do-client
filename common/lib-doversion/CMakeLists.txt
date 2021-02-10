
set(version_lib_name doversion)
add_library(${version_lib_name} STATIC do_version.cpp)
target_include_directories(${version_lib_name} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
if (DO_COMPONENT_NAME AND DO_COMPONENT_VERSION)
    add_component_version_definitions(${version_lib_name} ${DO_COMPONENT_NAME} ${DO_COMPONENT_VERSION})
else ()
    message (FATAL_ERROR "Component info not set. Make sure it is set before adding this library.")
endif ()
